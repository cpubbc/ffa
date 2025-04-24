#ifndef __FFACOREC_H__
#define __FFACOREC_H__

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define BFU_SZ_SFT 3
#define BFU_ROUNDUP(s) (((s - 1) >> BFU_SZ_SFT) + 1)
#define CLS_MAX 5

#define DEBUG 0

#if DEBUG

#define DBG(...)                                                               \
  do {                                                                         \
    char msg[1024];                                                            \
    sprintf(msg, __VA_ARGS__);                                                 \
    printf("[ffa] %s: %s\n", __FUNCTION__, msg);                               \
  } while (0)
#else
#define DBG(...)
#endif

struct cls_table {
  int tla_sk;
  uint64_t gmask;
  int gla_sk;
};

extern const struct cls_table cls_table[];
size_t get_tla_spec_size(int cls);
size_t get_gla_spec_size(int cls);

static inline size_t get_tla_spec_bfu(int cls) {
  return (get_tla_spec_size(cls) >> BFU_SZ_SFT);
}
static inline size_t get_gla_spec_bfu(int cls) {
  return (get_gla_spec_size(cls) >> BFU_SZ_SFT);
}
static inline size_t bfu2size(size_t bfu) { return (bfu << BFU_SZ_SFT); }
static inline size_t size2bfu(size_t size) { return (size >> BFU_SZ_SFT); }

static inline bool try_cas64(uint64_t *p, uint64_t *o, uint64_t n) {
  return __atomic_compare_exchange_n(p, o, n, false, __ATOMIC_ACQ_REL,
                                     __ATOMIC_ACQUIRE);
}

static inline int get_cls(size_t size) {
  uint64_t t = __builtin_clzll(size);
  return 5 - (((53 - t) >> 63) + ((49 - t) >> 63) + ((45 - t) >> 63) +
              ((43 - t) >> 63) + ((41 - t) >> 63));
}

#endif // __FFAALLOC_H__
