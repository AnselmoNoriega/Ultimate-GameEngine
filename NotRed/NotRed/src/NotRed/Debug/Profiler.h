#pragma once

#include <optick.h>
#define NR_ENABLE_PROFILING 1

#if NR_ENABLE_PROFILING
    #define NR_PROFILE_FRAME(...)           OPTICK_FRAME(__VA_ARGS__)
    #define NR_PROFILE_FUNC(...)            OPTICK_EVENT(__VA_ARGS__)
    #define NR_PROFILE_TAG(NAME, ...)       OPTICK_TAG(NAME, __VA_ARGS__)
    #define NR_PROFILE_SCOPE_DYNAMIC(NAME)  OPTICK_EVENT_DYNAMIC(NAME)
    #define NR_PROFILE_THREAD(...)          OPTICK_THREAD(__VA_ARGS__)
#else
    #define NR_PROFILE_FRAME(...)
    #define NR_PROFILE_FUNC()
    #define NR_PROFILE_TAG(NAME, ...) 
    #define NR_PROFILE_SCOPE_DYNAMIC(NAME)
    #define NR_PROFILE_THREAD(...)
#endif