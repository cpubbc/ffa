#include "ffa_core.h"
#include <sys/types.h>

typedef struct global_arena {
    volatile uint64_t tlamap;
    int               cls;
    uint8_t*          arena;
} gla_t;

extern const struct cls_table cls_table[];

extern void init_tla(void* t, int idx, int cls);

// depends on cls level
gla_t gla_blocklist[CLS_MAX];

static gla_t* get_gla(int cls) { return &gla_blocklist[cls]; }

void* alloc_tla(int cls)
{
    gla_t* g   = get_gla(cls);
    bool   r   = false;
    int    idx = 0;

    if (!g->arena)
    {
        g->arena = malloc(get_gla_spec_size(cls));
        assert(g->arena);
    }
    do
    {
        volatile uint64_t o = g->tlamap;
        if (!o) return NULL;

        idx        = __builtin_ctzll(o);
        uint64_t n = o & ~(1ULL << idx);
        DBG("idx:%d,new map:%llx\n", idx, n);
        r = try_cas64(&g->tlamap, &o, n);

    } while (!r);

    void* tla = &g->arena[idx * get_tla_spec_size(cls)];
    init_tla(tla, idx, cls);
    DBG("return tla:%p", tla);
    return tla;
}

void free_tla(int cls, int idx)
{
    gla_t* g = get_gla(cls);
    bool   r = false;
    do
    {
        volatile uint64_t o = g->tlamap;
        uint64_t          n = o | (1ULL << idx);
        r                   = try_cas64(&g->tlamap, &o, n);
    } while (!r);
}

void* search_tla(void* addr)
{
    int    hit = 0;
    int    cls = 0;
    gla_t* g   = NULL;
    for (int i = 1; i < CLS_MAX + 1; i++)
    {
        uint64_t start = (uint64_t)gla_blocklist[i - 1].arena;
        uint64_t size  = get_gla_spec_size(i - 1);
        hit = ((uint64_t)addr >= start) && ((uint64_t)addr < start + size);
        cls += hit * i;
    }
    if (!cls--) return NULL;
    DBG("Find cls:%d\n", cls);
    g        = get_gla(cls);
    int tlac = ((uint64_t)addr - (uint64_t)g->arena) >>
               (__builtin_ctzll(get_tla_spec_size(cls)));
    void* p = (void*)((uint64_t)g->arena + tlac * get_tla_spec_size(cls));
    DBG("Get tlac:%d,%p,%p\n", tlac, p, addr);

    return p;
}

static __attribute__((constructor)) void init_gla_arena(void)
{
    for (int i = 0; i < CLS_MAX; i++)
    {
        gla_t* g = get_gla(i);
        g->arena = malloc(get_gla_spec_size(i));
        __atomic_store_n(&g->tlamap, cls_table[i].gmask, __ATOMIC_RELEASE);
        DBG("gmask:%llx\n", g->tlamap);
    }
}
