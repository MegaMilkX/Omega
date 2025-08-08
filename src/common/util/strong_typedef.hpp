#pragma once



#pragma pack(push, 1)
template<typename T, typename TAG>
class strong_typedef {
    T value;
public:
    strong_typedef()
        :value(T()) {}
    strong_typedef(const T& v)
        : value(v) {}

    strong_typedef& operator=(const T& v) {
        value = v;
        return *this;
    }

    operator const T() const {
        return value;
    }
};
#pragma pack(pop)


#define STRONG_TYPEDEF(TYPE, ALIAS) \
    struct ALIAS ## _tag {}; \
    typedef strong_typedef<TYPE, ALIAS ## _tag> ALIAS