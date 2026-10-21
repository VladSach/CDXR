#ifndef CHAOS_ASSERT_H
#define CHAOS_ASSERT_H

#include <iostream>

#define STATIC_ASSERT static_assert

#if defined(_MSC_VER)
    #define CHBREAK __debugbreak()
#elif defined(__clang__) || defined(__GNUC__)
    #if __has_builtin(__builtin_debugtrap)
        #define CHBREAK __builtin_debugtrap()
    #else
        #include <signal.h>
        #if defined(SIGTRAP)
            #define CHBREAK raise(SIGTRAP)
        #else
            #define CHBREAK raise(SIGABRT)
        #endif
    #endif
#else
    #define CHBREAK asm("int $3")
#endif

#define PANIC(title, reason, ...)         \
{                                         \
    std::cout << title << reason << '\n'; \
    CHBREAK;                              \
}

#define ALWAYS_ASSERT(expression, ...)                           \
    if (!(expression)) {                                         \
        PANIC("Assertion failed: ", #expression, ##__VA_ARGS__); \
    }

#endif // CHAOS_ASSERT_H
