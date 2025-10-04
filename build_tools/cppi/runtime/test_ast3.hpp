

[[cppi_begin]];

template<typename T0 = int, typename T1>
struct TS;

template<typename T0, typename T1 = float>
struct TS {};

template<typename T>
struct MyTemplate {
	
};
/*
namespace N {
	template<typename T>
	struct S;
}

template<typename T>
struct N::S {
	
};*/

struct S3 {};

struct S {
	struct S2* a;
	struct S2;
	S2* b;
	S3* c;
};

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

[[cppi_end]];