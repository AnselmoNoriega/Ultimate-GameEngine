#pragma once

#ifdef NR_DEBUG
    #define NR_ENABLE_ASSERTS
#endif

#ifdef NR_ENABLE_ASSERTS
    #define NR_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { NR_ERROR("Assertion Failed"); __debugbreak(); } }
    #define NR_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { NR_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }
    
    #define NR_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
    #define NR_GET_ASSERT_MACRO(...) NR_EXPAND_VARGS(NR_ASSERT_RESOLVE(__VA_ARGS__, NR_ASSERT_MESSAGE, NR_ASSERT_NO_MESSAGE))
    
    #define NR_ASSERT(...) NR_EXPAND_VARGS( NR_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
    #define NR_CORE_ASSERT(...) NR_EXPAND_VARGS( NR_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
    #define NR_ASSERT(...)
    #define NR_CORE_ASSERT(...)
#endif