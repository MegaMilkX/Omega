#pragma once

// TODO LIST:

// File groups can be handled in preprocessor only
// Remove knowledge of files from pp_state, make it just a token reader
// move macros to preprocessor class
// pass preprocessor to the initial functions
// macro expand constant expressions before starting to evaluate?
// 		That way macro expansion should stay only in top level functions

// Parsing pp_number in constant expressions
//		pp Contant expressions are broken in general

// Handle basic pp tokens parsing better (without 'rules' and recursion)
//		rs = rs_best; Should be unnecessary
// handle 1'00 in constant-expression
// handle 'c' literals in constant-expression
// find out if need to handle 10e+
// 4. Macro redefinitions
// 5. Fn-like argument mismatch
//		Post a warning, don't fail

// _Pragma operator
// #line
// #pragma once
// TODO: Allow trailing , in macro arguments (last one just going to be an empty arg)


// Normal chars: #\^\
[]|\
{}~
// Trigraphs   : ??=??/??'\
??(??)??!\
??<??>??-
// Backslash+newline with a trigraph: Hello ??/
comment

#line 19 __FILE__

#if __cplusplus == 201402L
	"This implementation's version is 201402L";
	#if 1
		YES;
	#elif 1
		NO;
	#endif
#endif

// Predefined macros
long long version    = __cplusplus;
const char* date     = __DATE__;
const char* file     = __FILE__;
int line             = __LINE__;
int stdc_hosted      = __STDC_HOSTED__;
const char* time     = __TIME__;

#define INCL_FILE <include_me2.hpp>
#include <include_me.hpp>
#include INCL_FILE
#include "include_me.hpp"
#include "include_me2.hpp"

#define MACRO(A, B, C) A##B##C;
MACRO(X, Y,9)

#define CC X ## 9
CC;

#define X(N)
#define STRINGIFY(STR) # STR
STRINGIFY(
    This
    is
    'a'
    test
    "C:\\some\path\ "
);

#define GLUE(A, B) A##B
GLUE(X_, STRINGIFY(X(1)));

#define FN_MACRO(A, ...) \
function_call(A, __VA_ARGS__)

printf("%s, line %i", __FILE__, __LINE__);

#define ABC(A, B, C) A,B,C
ABC(1, 2);

FN_MACRO(13 , NULL, pair(1, 2), MyClass<int>(), NULL);

#define EXPRESSION_0 \
1 ? !!!(0) + 2 * (1 << 2) : 13

#if EXPRESSION_0
int SUCC = 1;
#endif

#define C

#if defined(A)
int A_ = 0;
#elif defined(B)
int B_ = 0;
#elif defined(C)
int C_ = 0;
#elif defined(D)
int D_ = 0;
#endif

#define FOO
#if defined FOO
float pi = 3.14f;
#endif

#define TEST
#undef TEST
#define HELLO hello_expanded

#ifdef TEST
const char* msg = "TEST is defined";
#else
#ifdef HELLO
const char* msg = "TEST is not defined, but HELLO is";
#else
const char* msg = "TEST is not defined";
#endif
#endif

#define hash_hash # ## #
#define mkstr(a) # a
#define in_between(a) mkstr(a)
#define join(c, d) in_between(c hash_hash d)
hash_hash;
in_between(c hash_hash d);
char p[] = join(x, y);

#define FOO1(A) A
#define FOO2(A, B) A, FOO1(B)
#define FOO3(A, B, C) A, FOO2(B, C)
#define FOO4(A, B, C, D) A, FOO3(B, C, D)

int FOO4(x, y, z, w);

#define CONCAT(A, B) A ## B
int CONCAT(CONCATENATED, _ARGUMENTS);

STRINGIFY(Hello!);

#define TABSIZE 100
int table[TABSIZE];

#define max(a, b) ((a) > (b) ? (a) : (b))
max(1, 2);

#define FOO(TYPE, NAME) \
class NAME \
{ \
	TYPE NAME; \
}

FOO(int, var);

#define BAZ BAR
#define BAR 3
#define FOO BAZ

FOO;

#define B z2
#define FOO2 x,b
#define FOO(A, B, C) int A; int B; int C;

FOO(FOO2, y)
FOO(FOO2, y, z)
FOO(FOO2, y, z, w)

#define FOO

#define FN_0() FN_0_EXPANDED
#define FN_1(...) FN_1 __VA_ARGS__
#define FN_2(A) FN_2 A
#define FN_3(A, B, C) FN_3 A B C
#define FN_4(A, B, C, ...) FN_4 A B C __VA_ARGS__

FN_0 ();
FN_1(int(1), (2, (x,y)), 3);
FN_2(1);
FN_3(1, 2, 3);
FN_4(1, 2, 3, 4);

#define Z "test"
#define A HELLO C
#define B WORLD Z
#define C A B

C

#
#define ABC _abc_expanded_
#warning

ABC#include <abc>

#ifndef ABC
int a;
#else
int b;
#endif

// Hello, this is a comment
/*
	Multi
	line
	comment
*/

MyTemplate<::OtherType> var;
<::>

int main() {
    int var = 1;
    var |= 1 << 8;
    return 0;
}
/*
*/
/**/

56'100
0xFF
0XDEAD'BEEF
0b1100'1010
0B1100'1010
0754
.1
3.14
2.
1.23e-4
44e+4
3e2

3.2
.0f
1.F
100
0xf00d
0b1001
06
'c'
''
U'a'
U'\''
'\'' '\"' '\?' '\\'
'\a' '\b' '\f' '\n' '\r' '\t' '\v'
'\056'
'\xf00d'

"hello"
L"\tHello, \"World!\""

LR"delim(Hello, \
World! ??=)delim"

hi
