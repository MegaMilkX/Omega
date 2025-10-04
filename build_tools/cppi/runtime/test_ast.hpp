
#define CPPI_CLASS	[[cppi_class]]; class
#define CPPI_STRUCT [[cppi_class]]; struct
#define CPPI_UNION	[[cppi_class]]; union

//Foo<(Bar<int>) > baz;
Foo<sizeof(int)> baz5;
Foo<(1 > 0)> baz3;
Foo<(1 >> 1)> baz4;
Foo<Bar<int>> baz1;
Foo<Bar<1>> baz2;

[[cppi_begin]];

enum {
	A, B, C
} e;

enum E;
enum E {
	Q, W, E
};

struct {} s;

struct S;
struct S {
};

template<typename T>
class MyTemplate {
	
};

class FOO final : BAR {
};
class FOO final;

struct Record {};

namespace nested {
	enum ::ENUM1;
	enum ENUM2; // elaborated-type-specifier
	enum class ENUM2; // opaque-enum-declaration
	enum ENUM3 : int; // opaque-enum-declaration
	class NestedForwardDecl;
	
	void my_func(void) {
		return;
	}
}

[[cppi_end]];

[[cppi_decl]];
using PFN_MYFUNC_T = int(*)(float, float);

inline namespace NAMESPACE {
	[[cppi_enum]];
	enum class TYPE : int {
		TYPE_A = 0,
		TYPE_B = 0x0010,
		TYPE_C = 0x0020,
	};
	
	template<typename T, int COUNT>
	class Array {
		int size = 0;
		T data[COUNT];
	public:
		static int CAPACITY = COUNT;
		
	};
	
	template<typename T>
	void foo3(T* a) {}
	
	int a = 10;
	float foo() {
		return .0f;
	}
	int bar() try {
		// hehe
	} catch(std::exception& ex) {
		// haha
	}
}

CPPI_CLASS MyClass : public BaseA<int, 1>, BaseB {
	const int param = 0;
	char a = 'a', b;
	const char* str = "hello";
public:
	[[cppi_class]];
	struct nested_class {};
	[[cppi_enum]];
	enum nested_enum {
		nested_a,
		nested_b,
		nested_c
	};

	MyClass();
	MyClass()
		: param(0), a('a'), b('q') {}
	MyClass() = default;
	MyClass() = delete;
	MyClass foo2();
	MyClass* next = 0; // TODO: nullptr not handled

	virtual int foo() = 0;
	int bar() const override final;
	
	[[cppi_decl]]
	int my_func() {
		return 0;
	}
	
	[[cppi_decl]]
	int my_func2() try {
		// hehe
	} catch(std::exception& ex) {
		// haha
	}
};
;
CPPI_STRUCT MyStruct{
	int a : 1;
	int : 3;
};

CPPI_UNION MyUnion{};
