#pragma once

#include "Types.h"
#include <cassert>

#ifdef HC_DEBUG
    #define HC_ENABLE_ASSERTS
#endif

#ifdef HC_ENABLE_ASSERTS
    #define HC_ASSERT(x, ...) \
        do { \
            if (!(x)) { \
                HC_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); \
                assert(false && #x); \
            } \
        } while (false)
    #define HC_CORE_ASSERT(x, ...) \
        do { \
            if (!(x)) { \
                HC_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__); \
                assert(false && #x); \
            } \
        } while (false)
#else
    #define HC_ASSERT(x, ...)
    #define HC_CORE_ASSERT(x, ...)
#endif

// Static assert (compile time)
#define HC_STATIC_ASSERT(x, msg) static_assert(x, msg)

// Verify (always evaluates expression, only asserts in debug)
#ifdef HC_DEBUG
    #define HC_VERIFY(x) HC_ASSERT(x, "Verify failed")
#else
    #define HC_VERIFY(x) (void)(x)
#endif
