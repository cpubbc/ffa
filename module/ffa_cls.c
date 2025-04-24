#include "ffa_core.h"
const struct cls_table cls_table[CLS_MAX] = {{8, ~(0ULL), 512},
                                             {128, ~(0ULL), 8 * 1024},
                                             {2048, ~(0ULL), 128 * 1024},
                                             {8192, ~(0ULL), 512 * 1024},
                                             {32768, 0xFFFFFFF, 1024 * 1024}};
size_t get_tla_spec_size(int cls) { return cls_table[cls].tla_sk << 10; }

size_t get_gla_spec_size(int cls) { return cls_table[cls].gla_sk << 10; }
