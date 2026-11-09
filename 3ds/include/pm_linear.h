#ifndef PM_LINEAR_H
#define PM_LINEAR_H
/* LINRACE: the linear heap with one lock. libctru 2.x linearAlloc, linearMemAlign, linearFree and linearRealloc keep the blocks in an
 * rbtree (AddrMap) and a MemPool and take no lock: two threads in them at once corrupt the tree (XL crash 2026-11-09, prefetch thread in
 * linearMemAlign). Every linear call of the port goes through these; the link wraps the library's own calls too (CMakeLists --wrap). */
#include <stddef.h>
void* pm_linear_alloc(size_t size);
void* pm_linear_memalign(size_t size, size_t align);
void pm_linear_free(void* p);
void* pm_linear_realloc(void* p, size_t size);
#endif
