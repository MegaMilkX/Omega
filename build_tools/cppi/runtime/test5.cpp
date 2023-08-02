/*
	The declaration in a template-declaration shall
	—(1.1) declare or define a function, a class, or a variable, or
	—(1.2) define a member function, a member class, a member enumeration, or a static data member of a class
	template or of a class nested within a class template, or
	—(1.3) define a member template of a class or class template, or
	—(1.4) be an alias-declaration.
*/
/*
	A type-parameter whose identifier does not follow an ellipsis defines its identifier to be a typedef-name (if
	declared with class or typename) or template-name (if declared with template) in the scope of the template
	declaration. [ Note: Because of the name lookup rules, a template-parameter that could be interpreted as
	either a non-type template-parameter or a type-parameter (because its identifier is the name of an already
	existing class) is taken as a type-parameter.
*/
/*
	A non-type template-parameter shall have one of the following (optionally cv-qualified) types:
	—(4.1) integral or enumeration type,
	—(4.2) pointer to object or pointer to function,
	—(4.3) lvalue reference to object or lvalue reference to function,
	—(4.4) pointer to member,
	—(4.5) std::nullptr_t.
*/

#include "include_me.hpp"
#include <vector>
//#include <windows.h>

#include "reflect_me.hpp"

template<typename T>
class TemplateClass;
template<typename T>
class TemplateClass{
	void member_fn() {}
	class member_class {};
	enum member_enum {};
	static int member_static_data_member;
};
template<typename T>
void TemplateFunction();
template<typename T>
void TemplateFunction() {}

template<typename T>
T TemplateVariable = 3.14;/*
class normalClass {
	template<typename T>
	class TemplateClass;
};*/
int;

TemplateClass<int> tpl_object;

/*
auto (*trailing_ret_)()->int;
auto bar()->auto (*)()->int, foo()->short;
*/

int (*X0);
int (*X1)();
int *X2 ();
int (*(&X3)())();
int (*(&X4)());
int (*(&X5))();
//int X6[]();
//int X7()[];
//int X8()();
int (*X9[])();
int (*X10())();

/*
char c; // char
unsigned char uc; // unsigned char
signed char sc; // signed char
char16_t c16; // char16_t
char32_t c32; // char32_t
bool b; // bool
unsigned u; // unsigned int
unsigned int ui; // unsigned int
signed s; // int
signed int si; // int
int i; // int
unsigned short int ushi; // unsigned short int
unsigned short ush; // unsigned short int
unsigned long int uli; // unsigned long int
unsigned long ul; // unsigned long int
unsigned long long int ulli; // unsigned long long int
unsigned long long ull; // unsigned long long int
signed long int sli; // long int
signed long sl; // long int
signed long long int slli; // long long int
signed long long sll; // long long int
long long int lli; // long long int
long long ll; // long long int
long int li; // long int
long l; // long int
signed short int sshi; // short int
signed short ssh; // short int
short int shi; // short int
short sh; // short int
wchar_t wc; // wchar_t
float f; // float
double d; // double
long double ld; // long double
void v; // void
auto auto_ = 0; // placeholder for a type to be deduced
*/
class {};

/*
class
[[cppi_reflect]]
MyClass : public MyBase {
	[[ cppi_reflect ]]
	int parameter;
	
	[[ cppi_getter(parameter) ]]
	int getParameter(){ return parameter; }
	[[ cppi_setter(parameter) ]]
	void setParameter(int param) { parameter = param; }
	
	class C {};
	
	[[reflect]];
	: 0;
	
	int a : 8;
	int x = 0, y = 0;
	int array_[];
	void undefined_fn();
public:
	[[reflect]]
	int foo() {
		return 0;
	}
};
const MyClass my_object = 0;
const MyClass* kekw() { return 0; }

[[reflect]]
struct A {} a;

int (*a_ptr) = 0, (b_obj) = 0, c_obj = 0;

void foo(int a_param, float b_param, unsigned c_param) {}

auto bar(int b_param_)-> void {}
int arr[1][2][3];

[[reflect, serialize, range(.0, 1.)]]
[[attrib...]]
int* my_func(int param) {
	return 0;
}

int main(int argc, char* argv[]) {
	return 0;
}

typedef long long INT64;
typedef float FUNC(int, char);

INT64* my_fun(void);
INT64* my_fun(void) { return 0; }
INT64* my_fun(int x) { return 1; }
void   my_fun(int x, int u) { return 1; }
FUNC func_ {
	return .0f;
}
*/

class Enclosing {
public:
	//std::vector<int> foo2() {}
	TYPE foo(TYPE t) {}
	const Nested* member;
	auto bar()->Nested*;
	
	class Nested {
		int value = 0;
	};
	
	typedef int TYPE;
};




