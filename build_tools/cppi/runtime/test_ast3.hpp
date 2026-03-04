
#include "somebs.hpp"

#define TEST_SELECTIVE_PARSING 1
#define TEST_VARIADIC_TEMPLATES 1
#define TEST_VARIOUS_JUNK 1
#define TEST_ELABORATED_SPECIFIERS 1
#define TEST_CONST_EXPR 1
#define TEST_CRTP 1
#define TEST_RECURSIVE_TEMPLATE 0

// TODO:
// - parse declarator member pointer part
// - nested-name-specifiers might not be fully resolved,
// while instantiating templates, need to check
//		- anything after T can't be a namespace,
//		but I remember putting plain identifiers there.
//		Should be ast::unresolved_name or something
// ! don't fully instantiate templates where it's not required
// ! variadic template non-type parameters
// - function overload declaration check
// - better error reporting
//		- ast nodes should store location
//			- can get recursively from leaves,
//			but they all need to keep their tokens
// - remove old code
// - remove all the *_2 suffixes
// - qualified-id
// - nested-name-specifiers in class and enum definitions
// - recheck correct re/declaration behavior after unqualified-id update
//		- check if simple-template-id is handled around there too
// - parameters-and-qualifiers -> trailing-return-type
//		- ew
// - change printfs to logs
//		- don't remove them, but put under a verbose flag
// - Cache failed potential #include paths

// BONUS:
// - Support constexpr "variables"
// - Support sizeof()
//		- Needs expression type deduction
//		- Needs user defined type entity storage separate from symbols
// - Support decltype()
//		- Needs expression type deduction
// - template explicit specializations
// - template partial specializations

#if TEST_SELECTIVE_PARSING
[[cppi_begin]];
namespace nlohmann {
	class json;
};
[[cppi_end]];

[[cppi_class]];
class Rectangle {
public:
	[[cppi_decl, set("width")]]
	void setWidth(float w);
	[[cppi_decl, set("height")]]
	void setHeight(float h);
	[[cppi_decl, get("width")]]
	float getWidth() const;
	[[cppi_decl, get("height")]]
	float getHeight() const;
	
	[[cppi_decl]]
	int color = 0;
	
	[[cppi_decl, serialize_json]]
	void toJson(nlohmann::json& j) {}
	[[cppi_decl, deserialize_json]]
	void fromJson(const nlohmann::json& j) {}
};
#endif

[[cppi_begin]];

#if TEST_VARIOUS_JUNK

struct SOME_STRUCT {
	class Nested {
		
	};
	
	typedef const Nested* TYPE;
	
	TYPE c;
	
	void set(int);
	void set(float);
	
	void Rectangle() {}
	SOME_STRUCT() {}
	
	void foo() {}
};
enum SOME_ENUM {
	SOME_A = 67,
	SOME_B,
	SOME_C
};

SOME_STRUCT::Nested* ssn = 0;

namespace NAMESPACE {}
namespace NAMESPACE {
	template<typename T>
	struct Vector {};
	
	typedef Vector<int*> my_vec;
}

template<typename T>
struct MyTemplateBase {};

template<typename T, int I = 10>
struct MyTemplate;
template<typename T = int, int I>
struct MyTemplate;

typedef MyTemplateBase<int> BASE_T;

template<typename T, int I>
struct MyTemplate : public MyTemplateBase<T> {
	//T* member;
};

template<typename T>
class Vector {
	int capacity = 0;
	int size = 0;
	T* data = nullptr;
public:
	void push_back(const T& value);
	const T& get(int i) const;
	T& get(int i);
};

template<typename T, int CAP>
class Array {
	const int CAPACITY = CAP;
	int size = 0;
	T* data = nullptr;
public:
};

typedef int*(*pfn_func_t)(void*, int);
pfn_func_t my_pfn = 0;

//MyTemplate<const int* const(* const)(int, void*), 0> my_template;
MyTemplate<> my_template() {
	
}
MyTemplate<long double, 33> my_template2;
MyTemplate<wchar_t[10], 23> my_template3;
MyTemplate<void(*)() const volatile, 42> my_template4;
MyTemplate<int*, 42> my_template5;
MyTemplate<const SOME_STRUCT*, 99> my_template6;
MyTemplate<struct SOME_STRUCT*, 99> my_template7;
MyTemplate<SOME_ENUM*, 85> my_template8;
MyTemplate<enum SOME_ENUM*, 85> my_template9;
MyTemplate<Vector<int>, SOME_C> my_template10;
Vector<int> my_vector;
Array<float, SOME_C> my_float_array;

using MY_TYPE = void(*)();

Array<MY_TYPE, SOME_A> my_something_array;

#endif

#if TEST_VARIADIC_TEMPLATES

template<typename T>
class VBase {};

template<typename T, typename... ARGS>
class VariadicTemplate : VBase<T>, VBase<ARGS>... {
	
};

VariadicTemplate<int> my_variadic;
VariadicTemplate<int, float> my_variadic2;
VariadicTemplate<int, float, char> my_variadic3;
VariadicTemplate<int, float, char, void*> my_variadic4;
VariadicTemplate<int, float, char, void*, unsigned> my_variadic5;
VariadicTemplate<const int, const Vector<int>, const Vector<int>*, void* const, const int* const> my_variadic6;

// _Z16VariadicTemplateIiJfcPvjEE
//   16VariadicTemplateIiJfcPvjEE
#endif

#if TEST_ELABORATED_SPECIFIERS

struct S {
	struct S2* a;
	struct S2;
	S2* b;
};

#endif

#if TEST_CRTP

template<typename T>
class CRTP_Base {
	T* beeba = nullptr;
	T* bobba(const T& arg) {}
};

class CRTP_User : public CRTP_Base<CRTP_User> {
};

#endif

#if TEST_RECURSIVE_TEMPLATE

// TODO:
template<typename T, typename... ARGS>
class RECUR_Base : RECUR_Base<ARGS...> {
    
};

template<typename T>
class RECUR_Base<T> {

};

class RECUR : public RECUR_Base<int, float, char, void*> {};

#endif

#if TEST_CONST_EXPR
int i = .0F + .0L + 1U + 1L + 1LU + 1LLU + 1UL + 1ULL;

static_assert((10 + 3) / 2 + (0x0010 | 0x0001), "foo");
//static_assert(0, "failure");

// Boolean literals
static_assert(true, "true");
static_assert(false != true, "false");

// Basic integer constants
static_assert(1 == 1, "simple equality");
static_assert(42 != 43, "simple inequality");
//static_assert('A' == 65, "character literal value");

// Unary operators
static_assert(-5 == (0 - 5), "unary minus");
static_assert(+7 == 7, "unary plus");
static_assert(!false, "logical not");

// Arithmetic
static_assert(2 + 3 == 5, "addition");
static_assert(9 - 4 == 5, "subtraction");
static_assert(6 * 7 == 42, "multiplication");
static_assert(8 / 2 == 4, "division");
static_assert(8 % 3 == 2, "modulus");

// Precedence & parentheses
static_assert(2 + 3 * 4 == 14, "operator precedence");
static_assert((2 + 3) * 4 == 20, "parentheses grouping");

// Shifts
static_assert((1 << 3) == 8, "left shift");
static_assert((16 >> 2) == 4, "right shift");

// Bitwise
static_assert((6 & 3) == 2, "bitwise and");
static_assert((6 | 3) == 7, "bitwise or");
static_assert((6 ^ 3) == 5, "bitwise xor");
static_assert(~0 == -1, "bitwise not");

// Relational
static_assert(5 < 10, "less than");
static_assert(10 > 5, "greater than");
static_assert(7 <= 7, "less equal");
static_assert(9 >= 8, "greater equal");

// Equality
static_assert(7 == 7, "equal");
static_assert(7 != 8, "not equal");

// Logical binary
static_assert(13 && true, "logical and");
static_assert(13 || false, "logical or");

// Conditional (ternary)
//static_assert((1 ? 42 : 99) == 42, "ternary true branch");
//static_assert((0 ? 42 : 99) == 99, "ternary false branch");

// Mixed
static_assert(((2 + 3) * (10 - 4)) == 30, "compound arithmetic");
static_assert((1 + 2 + 3 + 4 + 5) == 15, "summation");

// Integral promotions & literals
static_assert(1000000000LL > 1000, "long long literal");
static_assert(0x10 + 010 == 24, "hex + octal");
//static_assert('Z' - 'A' == 25, "char subtraction");
#endif
[[cppi_end]];