#ifndef __FFAAPI_H__
#define __FFAAPI_H__
#include <stddef.h>

void *alloc_fragment(size_t size);
void free_fragment(void *p);

#endif
