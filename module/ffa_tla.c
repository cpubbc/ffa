#include "ffa_core.h"
#include "threads.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define SANITY_CHECK 0

extern void* alloc_tla(int cls);
extern void  free_tla(int cls, int idx);
extern void* search_tla(void* addr);

typedef union thread_local_arena_meta {
    struct {
        uint64_t cls : 3;
        uint64_t idx : 6;
        uint64_t state : 1;
#if SANITY_CHECK
        uint64_t debit : 19;
        uint64_t credit : 19;
        uint64_t magic : 16;
#else
        uint64_t debit : 27;
        uint64_t credit : 27;
#endif
    } tag;
    volatile uint64_t cv;
} tla_meta_t;

typedef union thread_local_arena {
    tla_meta_t meta;
    uint8_t    arena[0];
} tla_t;

typedef struct thread_local_arena_control_block {
    tla_t* tla;
    size_t left_bfu;
    int    credit;
    int    cls;
} tla_cb_t;

#define tla_ready2free(meta)                                                   \
    (meta.tag.debit == meta.tag.credit && meta.tag.state)

thread_local tla_cb_t* tla_cb[CLS_MAX + 1]; // todo: last null to add

static void* free_bfu_pos(tla_cb_t* b)
{
    uint8_t* tail = &b->tla->arena[get_tla_spec_size(b->cls)];
    return (void*)(tail - bfu2size(b->left_bfu));
}

static void reset_tla(tla_t* tla)
{
    tla_meta_t m = {0};
    m.cv         = tla->meta.cv;
    m.tag.state  = 0;
    m.tag.credit = 0;
    m.tag.debit  = 0;
    __atomic_store_n(&tla->meta.cv, m.cv, __ATOMIC_RELEASE);
}

static int reset_tla_control_block(tla_cb_t* b)
{
    if (b->tla && tla_ready2free(b->tla->meta))
    {
        reset_tla(b->tla);
    }
    else
    {
        b->tla = alloc_tla(b->cls);
    }
    b->credit   = 0;
    b->left_bfu = b->tla ? get_tla_spec_bfu(b->cls) - 1 : 0;
    return b->tla ? 1 : 0;
}

static void tla_copy_on_full(tla_cb_t* b)
{
    bool       r = false;
    tla_meta_t m = {0};
    do
    {
        m.cv                = b->tla->meta.cv;
        volatile uint64_t o = b->tla->meta.cv;
        m.tag.state         = 1;
        m.tag.credit        = b->credit;
        r                   = try_cas64(&b->tla->meta.cv, &o, m.cv);

    } while (!r);
}

static void* alloc_fragment_recursive(tla_cb_t** pb, size_t size)
{
    int    fallback = 0;
    size_t need_bfu = BFU_ROUNDUP(size);
    // DBG("alloc size:%d, need_bfu:%d\n", size, need_bfu);
    if (!(*pb)) return NULL;
    DBG("Get CLS:%d\n", (*pb)->cls);

    if (!(*pb)->tla)
    {
        goto RECURSIVE;
    }

    else if ((*pb)->left_bfu < need_bfu)
    {
        tla_copy_on_full((*pb));
        goto RECURSIVE;
    }
    else
    {
        DBG("cls%d-idx%d\n", (*pb)->cls, (*pb)->tla->meta.tag.idx);
        void* p = free_bfu_pos(*pb);
        (*pb)->left_bfu -= need_bfu;
        (*pb)->credit++;
        return p;
    }
RECURSIVE:
    fallback = !reset_tla_control_block((*pb));
    return alloc_fragment_recursive(pb + fallback, size);
}

static void tla_clear_on_free(tla_t* t)
{
    bool       r = false;
    tla_meta_t m = {0};
#if SANITY_CHECK
    assert(t->meta.tag.magic == 0xdead);
#endif
    do
    {
        m.cv                = t->meta.cv;
        volatile uint64_t o = t->meta.cv;
        m.tag.debit++;
        r = try_cas64(&t->meta.cv, &o, m.cv);
    } while (!r);

    if (tla_ready2free(m))
    {
        // here, Allocator detach this tla cuz state = 1. We do free tla
        // procedure.
        DBG("Do tla:%d cls:%d free", m.tag.idx, m.tag.cls);
        free_tla(m.tag.cls, m.tag.idx);
    }
}

void* alloc_fragment(size_t size)
{
    if (!size) return NULL;
    if (!tla_cb[0])
    {
        for (int i = 0; i < CLS_MAX; i++)
        {
            tla_cb[i] = malloc(sizeof(tla_cb_t));
            assert(tla_cb[i]);
            tla_cb[i]->cls = i;
            reset_tla_control_block(tla_cb[i]);
        }
    }
    return alloc_fragment_recursive(&tla_cb[get_cls(size)], size);
}

void free_fragment(void* f)
{
    tla_t* t = search_tla(f);
    if (!t) return;
    tla_clear_on_free(t);
}

void init_tla(void* t, int idx, int cls)
{
    tla_meta_t m = {0};
    m.tag.state  = 0;
    m.tag.credit = 0;
    m.tag.debit  = 0;
    m.tag.cls    = cls;
    m.tag.idx    = idx;
#if SANITY_CHECK
    m.tag.magic = 0xdead;
#endif
    __atomic_store_n(&((tla_t*)t)->meta.cv, m.cv, __ATOMIC_RELEASE);
}
