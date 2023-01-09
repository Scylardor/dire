#pragma once

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

#define EXPAND(x) x
#define __DIRE_NARGS(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, VAL, ...) VAL
#define DIRE_NARGS_1(...) EXPAND(__DIRE_NARGS(__VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))

#define AUGMENTER(...) unused, __VA_ARGS__
#define DIRE_NARGS(...) DIRE_NARGS_1(AUGMENTER(__VA_ARGS__))

#else // Others

#define __DIRE_NARGS(_0, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, 15, N,...) N

#define DIRE_NARGS(...) __DIRE_NARGS(0, ## __VA_ARGS__, 14, 13, 12, 11, 10, 9, 8, 7, 6,5,4,3,2,1,0)

#endif
