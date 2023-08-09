#pragma once


#define CONCAT(A, B) A##B
#define EXPAND_AND_CONCAT(A, B) CONCAT(A, B)


#ifdef __COUNTER__
#define UNIQUE_IDENTIFIER(PREFIX) EXPAND_AND_CONCAT(PREFIX, __COUNTER__)
#else
#define UNIQUE_IDENTIFIER(PREFIX) EXPAND_AND_CONCAT(PREFIX, __LINE__)
#endif



#define STATIC_BLOCK_IMPL2(FUNC_NAME, VAR_NAME) \
    static void FUNC_NAME(); \
    static volatile int VAR_NAME = (FUNC_NAME(), 0); \
    static void FUNC_NAME()

#define STATIC_BLOCK_IMPL1(PREFIX) \
    STATIC_BLOCK_IMPL2(CONCAT(PREFIX, _fn), CONCAT(PREFIX, _var))


// Only use this in cpp or you will get multiple invocations
#define STATIC_BLOCK STATIC_BLOCK_IMPL1(UNIQUE_IDENTIFIER(_static_block_))