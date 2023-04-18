#ifndef _KERROR_MACRO_H__
#define _KERROR_MACRO_H__

#if defined(__GNUC__) || defined(__clang__)
#define KERROR_LIKELY(cond) __builtin_expect(!!(cond), 1)
#define KERROR_UNLIKELY(cond) __builtin_expect(!!(cond), 0)
#else
#define KERROR_LIKELY(cond) (cond)
#define KERROR_UNLIKELY(cond) (cond)
#endif

#endif
