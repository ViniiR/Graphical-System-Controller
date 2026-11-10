#ifndef _DEFER_H
#define _DEFER_H

#include <stdlib.h>

#if defined(__GNUC__)

#define _DEFER_INNER2(_a, _b) _a##_b
#define _DEFER_INNER(_a, _b) _DEFER_INNER2(_a, _b)
#define _DEFER_IMPL2(_fn, _var) \
    __extension__ auto void _fn(int *); \
    [[gnu::cleanup(_fn), maybe_unused]] int _var; \
    __extension__ auto void _fn([[maybe_unused]] int *_ptr)
#define _DEFER_IMPL(_n) \
    _DEFER_IMPL2(_DEFER_INNER(_defer_fn_, _n), _DEFER_INNER(_defer_var_, _n))
#define defer _DEFER_IMPL(__COUNTER__)

// static inline void _defree_cleanup(void *p) {
//     void **ptr = *(void ***)p;
//     if (ptr) {
//         free(ptr);
//     }
// }
// #define _DEFREE_IMPL(var, cnt) \
//     __attribute__((cleanup(_defree_cleanup))) [[maybe_unused]] void * _DEFER_CONCAT(_defree_var_, cnt) = (var)
// #define defree(ptr) _DEFREE_IMPL(ptr, __COUNTER__)

#else
#error "defer macro requires GCC"
#endif

#endif
