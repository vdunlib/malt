#pragma once

#ifdef __GNUC__

#ifndef likely
#define likely(x)   __builtin_expect(!!(x),1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x),0)
#endif

#else

#ifndef likely
#define likely(x)   x
#endif

#ifndef unlikely
#define unlikely(x) x
#endif

#endif // #ifdef __GNUC__

#define ALWAYS_INLINE __attribute__((always_inline)) inline
#define NO_INLINE    __attribute__((noinline))
#define HOT_PATH     __attribute__((hot))
#define COLD_PATH    __attribute__((cold))

#if defined(__clang__) || (__GNUC__ >= 8)
#define FALLTHROUGH [[fallthrough]];
#elif defined(__GNUC__)
#define FALLTHROUGH [[gnu::fallthrough]]
#else
#define FALLTHROUGH
#endif

#ifndef VDUNLIB_DISABLE_ALWAYS_INLINE
#define VDUNLIB_ALWAYS_INLINE ALWAYS_INLINE
#else
#define VDUNLIB_ALWAYS_INLINE inline
#endif
