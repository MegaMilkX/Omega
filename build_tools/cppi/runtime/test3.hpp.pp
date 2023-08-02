
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


// Normal chars: #\^[]|{}~
// Trigraphs   : #\^[]|{}~
// Backslash+newline with a trigraph: Hello comment


	"This implementation's version is 201402L";
		YES;
		NO;

// Predefined macros
long long version    = 201402L;
const char* date     = "May 13 2023";
const char* file     = "test3.hpp";
int line             = 54;
int stdc_hosted      = 0;
const char* time     = "16:11:23";

"include_me.hpp";
int include_me = 0;

"include_me2.hpp"
int include_me2 = 0;

"include_me.hpp";
int include_me = 0;

__FILE__
int include_me2 = 0;


XY9;

X9;

"This is 'a' test \"C:\\\\some\\path\\ \"";

X_STRINGIFY(X(1));


printf("%s, line %i", "test3.hpp", 86);

1,2,;

function_call(13, NULL, pair(1, 2), MyClass<int>(), NULL);


int SUCC = 1;


int A_ = 0;
int B_ = 0;
int C_ = 0;
int D_ = 0;

float pi = 3.14f;


const char* msg = "TEST is defined";
const char* msg = "TEST is not defined, but HELLO is";
const char* msg = "TEST is not defined";

##;
"c ## d";
char p[] = "x ## y";


int x, y, z, w;

int CONCATENATED_ARGUMENTS;

"Hello!";

int table[100];

((1) > (2) ? (1) : (2));


class var { 	int var; };


3;


int x,b; int y; int ;
int x,b; int y; int z;
int x,b; int y; int z, w;



FN_0_EXPANDED;
FN_1 int(1), (2, (x,y)), 3;
FN_2 1;
FN_3 1 2 3;
FN_4 1 2 3 4;


hello_expanded C WORLD "test"


_abc_expanded_#include <abc>

int a;
int b;

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
