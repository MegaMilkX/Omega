
#define CPPI_CLASS	[[cppi_class]]; class
#define CPPI_STRUCT [[cppi_class]]; struct
#define CPPI_UNION	[[cppi_class]]; union

[[cppi_begin]];

namespace std {
	template<typename T>
	class vector;
}


[[cppi_end]];

[[cppi_enum]];
enum TYPE {
	TYPE_ABOBA = 0,
	TYPE_FOO,
	TYPE_BAR,
	TYPE_BAZ
};

	
CPPI_CLASS MyBase {
public:
	
};


CPPI_CLASS MyClass : public MyBase {
public:
	[[cppi_decl]]
	std::vector<int> data;

	[[cppi_decl]]
	int number = 0;
	
	[[cppi_decl]]
	void foo(int, int) {}
	
	[[cppi_decl, set("value")]]
	void set_value(float v) {}
	[[cppi_decl, get("value")]]
	float get_value() const {}
};
