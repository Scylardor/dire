#pragma once

/* Double concatenation idiom in order to force the evaluation of a and b in case one of them is a macro
 * for more info, check out https://stackoverflow.com/questions/1489932/how-can-i-concatenate-twice-with-the-c-preprocessor-and-expand-a-macro-as-in-ar for example.
 */
#define DIRE_CONCAT(a, b) a ## b
#define DIRE_CONCAT2(a, b) DIRE_CONCAT(a, b)

#define DIRE_STRINGIZE(a) #a

/* Inspired from https://stackoverflow.com/a/26685339/1987466 */
/*
 * Count the number of arguments passed to ASSERT, very carefully
 * tiptoeing around an MSVC bug where it improperly expands __VA_ARGS__ as a
 * single token in argument lists. See these URLs for details:
 *
 *   http://connect.microsoft.com/VisualStudio/feedback/details/380090/variadic-macro-replacement
 *   http://cplusplus.co.il/2010/07/17/variadic-macro-to-count-number-of-arguments/#comment-644
 */
#ifdef _MSC_VER // Microsoft compilers

#define DIRE_EXPAND(x) x
#define DIRE_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, VAL, ...) VAL
#define DIRE_NARGS_1(...) DIRE_EXPAND(DIRE_NARGS_IMPL(__VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define AUGMENTER(...) unused, __VA_ARGS__
#define DIRE_NARGS(...) DIRE_NARGS_1(AUGMENTER(__VA_ARGS__))

#else // Others

#define DIRE_NARGS_IMPL(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N,...) N

#define DIRE_NARGS(...) DIRE_NARGS_IMPL(0, ## __VA_ARGS__, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6,5,4,3,2,1,0)

#endif

 // Stolen from https://stackoverflow.com/questions/48710758/how-to-fix-variadic-macro-related-issues-with-macro-overloading-in-msvc-mic
 // It's the approach that works the best for our use case so far.
#define DIRE_MSVC_BUG(MACRO, ARGS) MACRO ARGS  // name to remind that bug fix is due to MSVC :-)

/* Expands reliably a macro name with the appropriate suffix depending on the number of variable arguments provided.
 * Example: VA_MACRO(TEST, 1,2,3) is going to call the macro TEST3(1,2,3).
 */
#define DIRE_VA_MACRO(MACRO, ...) DIRE_MSVC_BUG(DIRE_CONCAT2, (MACRO, DIRE_NARGS(__VA_ARGS__)))(__VA_ARGS__)

/* Necessary to differentiate from a comma token (when we use this macro) from a comma delimiting macro arguments. */
#define DIRE_COMMA ,

#define DIRE_BRACES_STRINGIZE_COMMA_VALUE(a) { DIRE_STRINGIZE(a) DIRE_COMMA a } DIRE_COMMA

/* Hardcoded cascade of macros to declare ("value", value) pairs for up to 20 arguments (needed for enums). Need more? Feel free to expand the list */
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a1) DIRE_BRACES_STRINGIZE_COMMA_VALUE(a1)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE2(a1, a2) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a1) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a2)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE3(a1, a2, a3) DIRE_BRACES_STRINGIZE_COMMA_VALUE2(a1, a2) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a3)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE4(a1, a2, a3, a4) DIRE_BRACES_STRINGIZE_COMMA_VALUE3(a1, a2, a3) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a4)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE5(a1, a2, a3, a4, a5) DIRE_BRACES_STRINGIZE_COMMA_VALUE4(a1,a2,a3,a4) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a5)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE6(a1, a2, a3, a4, a5, a6) DIRE_BRACES_STRINGIZE_COMMA_VALUE5(a1,a2,a3,a4,a5) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a6)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE7(a1, a2, a3, a4, a5, a6, a7) DIRE_BRACES_STRINGIZE_COMMA_VALUE6(a1,a2,a3,a4,a5,a6) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a7)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE8(a1, a2, a3, a4, a5, a6, a7, a8) DIRE_BRACES_STRINGIZE_COMMA_VALUE7(a1, a2, a3, a4, a5, a6, a7) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a8)
#define  DIRE_BRACES_STRINGIZE_COMMA_VALUE9(a1, a2, a3, a4, a5, a6, a7, a8, a9) DIRE_BRACES_STRINGIZE_COMMA_VALUE8(a1, a2, a3, a4, a5, a6, a7, a8) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a9)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE10(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) DIRE_BRACES_STRINGIZE_COMMA_VALUE9(a1, a2, a3, a4, a5, a6, a7, a8, a9) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a10)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE11(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) DIRE_BRACES_STRINGIZE_COMMA_VALUE10(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a11)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE12(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) DIRE_BRACES_STRINGIZE_COMMA_VALUE11(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a12)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE13(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) DIRE_BRACES_STRINGIZE_COMMA_VALUE12(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a13)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE14(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) DIRE_BRACES_STRINGIZE_COMMA_VALUE13(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a14)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE15(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) DIRE_BRACES_STRINGIZE_COMMA_VALUE14(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a15)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE16(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) DIRE_BRACES_STRINGIZE_COMMA_VALUE15(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a16)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE17(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) DIRE_BRACES_STRINGIZE_COMMA_VALUE16(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a17)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE18(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) DIRE_BRACES_STRINGIZE_COMMA_VALUE17(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a18)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE19(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) DIRE_BRACES_STRINGIZE_COMMA_VALUE18(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a19)
#define DIRE_BRACES_STRINGIZE_COMMA_VALUE20(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20) DIRE_BRACES_STRINGIZE_COMMA_VALUE19(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) DIRE_BRACES_STRINGIZE_COMMA_VALUE1(a20)


#define DIRE_COMMA_ARGS_LSHIFT_COUNTER(a1) a1 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define DIRE_COMMA_ARGS_LSHIFT_COUNTER1(a1) DIRE_COMMA_ARGS_LSHIFT_COUNTER(a1)
#define DIRE_COMMA_ARGS_LSHIFT_COUNTER2(a1, a2, a3, a4) a1 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) DIRE_COMMA a2 = 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define DIRE_COMMA_ARGS_LSHIFT_COUNTER3(a1, a2, a3) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define DIRE_COMMA_ARGS_LSHIFT_COUNTER4(a1, a2, a3, a4) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a4= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)
#define DIRE_COMMA_ARGS_LSHIFT_COUNTER5(a1, a2, a3, a4, a5) a1= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) DIRE_COMMA a2= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) DIRE_COMMA a3= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET) DIRE_COMMA a4= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)  DIRE_COMMA a5= 1 << (__COUNTER__ - COUNTER_BASE-OFFSET)

#define DIRE_FIRST_TYPE_OR_REFLECTABLE_0() DIRE_NS::Reflectable2
#define DIRE_FIRST_TYPE_OR_REFLECTABLE_1(a, ...) a
#define DIRE_FIRST_TYPE_OR_REFLECTABLE_2(a, ...) a
#define DIRE_FIRST_TYPE_OR_REFLECTABLE_3(a, ...) a
#define DIRE_FIRST_TYPE_OR_REFLECTABLE_4(a, ...) a

#define DIRE_COMMA_ARGS1(a1) a1
#define DIRE_COMMA_ARGS2(a1, a2) a1 DIRE_COMMA a2
#define DIRE_COMMA_ARGS3(a1, a2, a3) a1 DIRE_COMMA a2 DIRE_COMMA a3
#define DIRE_COMMA_ARGS4(a1, a2, a3, a4) a1 DIRE_COMMA a2 DIRE_COMMA a3 DIRE_COMMA a4
#define DIRE_COMMA_ARGS5(a1, a2, a3, a4, a5) a1 DIRE_COMMA a2 DIRE_COMMA a3 DIRE_COMMA a4 DIRE_COMMA a5

// If one day, you wanna use IF_ELSE instead, read here http://jhnet.co.uk/articles/cpp_magic
// Important to use VA_MACRO here otherwise MSVC will expand whole va_args in a single argument
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE0(...) DIRE_NS::Reflectable2
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE1(...) DIRE_VA_MACRO(DIRE_COMMA_ARGS, __VA_ARGS__)
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE2(...) DIRE_VA_MACRO(DIRE_COMMA_ARGS, __VA_ARGS__)
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE3(...) DIRE_VA_MACRO(DIRE_COMMA_ARGS, __VA_ARGS__)
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE4(...) DIRE_VA_MACRO(DIRE_COMMA_ARGS, __VA_ARGS__)
#define DIRE_INHERITANCE_LIST_OR_REFLECTABLE5(...) DIRE_VA_MACRO(DIRE_COMMA_ARGS, __VA_ARGS__)

#ifdef _MSC_VER // Microsoft compilers
# define DIRE_FUNCTION_FULLNAME __FUNCTION__
#else
# define DIRE_FUNCTION_FULLNAME __func__
#endif