
{ identifierlist };
{ my, cool, list, of, identifiers };
{ a, b };
template<typename T>
class MyClass {
	int var0;
public:
	T* alloc() {
		auto p = new T();
		return p; 
	}
	void free(T* p) {
		delete p;
	}
	void do_stuff() {
		int x = 0xf00d;
		x << 3;
	}
};
