#ifndef GFXM_MATH_H
#define GFXM_MATH_H

#include <assert.h>
#include <math.h>
#include <cmath>
#include <limits>
#include <stdint.h>
#include <initializer_list>

namespace gfxm{

const float pi = 3.14159265359f;
const double d_pi = 3.14159265359;
    
// ====== Types ==========

template<typename T>
struct tvec2
{
    union { T x, r; };
    union { T y, g; };
    
    tvec2() : x(0), y(0) {}
    tvec2(T x, T y) : x(x), y(y) {}

    tvec2& operator=(const std::initializer_list<T>& l) {
        static_assert(l.size() <= 2, "tvec2 initializer list wrong size");
        for (int i = 0; i < l.size(); ++i) {
            operator[](i) = l[i];
        }
    }

    T operator[](const int &i) const {
        return *((&x) + i);
    }
    T& operator[](const int &i) {
        return *((&x) + i);
    }

    T length() const { return sqrt(x*x + y*y); }
    T length2() const { return x*x + y*y; }

    bool is_valid() const {
        if (isnan(x) || isinf(x)) return false;
        if (isnan(y) || isinf(y)) return false;
        return true;
    }
};

template<typename T>
struct tvec3
{
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };
    
    tvec3() : x(0), y(0), z(0) {}
    tvec3(T x, T y, T z) : x(x), y(y), z(z) {}
    tvec3(tvec2<T> xy, T z) : x(xy.x), y(xy.y), z(z) {}
    
    tvec3& operator=(const std::initializer_list<T>& l) {
        static_assert(l.size() <= 3, "tvec3 initializer list wrong size");
        for (int i = 0; i < l.size(); ++i) {
            operator[](i) = l[i];
        }
        return *this;
    }

    T operator[](const int &i) const {
        return *((&x) + i);
    }
    T& operator[](const int &i) {
        return *((&x) + i);
    }

    T length() const { return sqrt(x*x + y*y + z*z); }
    T length2() const { return x*x + y*y + z*z; }

    bool is_valid() const {
        if (isnan(x) || isinf(x)) return false;
        if (isnan(y) || isinf(y)) return false;
        if (isnan(z) || isinf(z)) return false;
        return true;
    }
};

template<typename T>
struct tvec4
{
    union { T x, r; };
    union { T y, g; };
    union { T z, b; };
    union { T w, a; };
    
    tvec4() : x(0), y(0), z(0), w(0) {}
    tvec4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    tvec4(const tvec3<T>& v3, T w)
    : x(v3.x), y(v3.y), z(v3.z), w(w) {}
    
    tvec4& operator=(const std::initializer_list<T>& l) {
        static_assert(l.size() <= 4, "tvec4 initializer list wrong size");
        for (int i = 0; i < l.size(); ++i) {
            operator[](i) = l[i];
        }
    }

    operator tvec3<T>() const { return tvec3<T>(x, y, z); }

    T operator[](const int &i) const {
        return *((&x) + i);
    }
    T& operator[](const int &i) {
        return *((&x) + i);
    }

    T length() const { return sqrt(x*x + y*y + z*z + w*w); }
    T length2() const { return x*x + y*y + z*z + w*w; }

    bool is_valid() const {
        if (isnan(x) || isinf(x)) return false;
        if (isnan(y) || isinf(y)) return false;
        if (isnan(z) || isinf(z)) return false;
        if (isnan(w) || isinf(w)) return false;
        return true;
    }
};

template<typename T>
struct tquat
{
    T x;
    T y;
    T z;
    T w;

    tquat<T>& operator=(const tvec4<T>& v) {
        x = v.x; y = v.y; z = v.z; w = v.w;
        return *this;
    }
    
    tquat() : x(0), y(0), z(0), w(1) {}
    tquat(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) {}
    tquat(const tvec4<T>& v) : x(v.x), y(v.y), z(v.z), w(v.w) {}
    
    bool is_valid() const {
        if (isnan(x) || isinf(x)) return false;
        if (isnan(y) || isinf(y)) return false;
        if (isnan(z) || isinf(z)) return false;
        if (isnan(w) || isinf(w)) return false;
        return true;
    }
};

template<typename T>
struct tmat3
{
    tmat3(){}
    explicit tmat3(T f)
    {
        col[0].x = f;
        col[1].y = f;
        col[2].z = f;
    }
    explicit tmat3(const tvec3<T>& col0, const tvec3<T>& col1, const tvec3<T>& col2) {
        col[0] = col0;
        col[1] = col1;
        col[2] = col2;
    }
    
    tvec3<T> operator[](const int &i) const {
        return col[i];
    }
    tvec3<T>& operator[](const int &i){
        return col[i];
    }

    bool is_valid() const {
        for (int i = 0; i < 3; ++i) {
            if (!col[i].is_valid()) {
                return false;
            }
        }
        return true;
    }
private:
    tvec3<T> col[3];
};

template<typename T>
struct tmat3x4;

template<typename T>
struct tmat4
{
    tmat4(){}
    explicit tmat4(T f)
    {
        col[0].x = f;
        col[1].y = f;
        col[2].z = f;
        col[3].w = f;
    }
    explicit tmat4(const tvec4<T>& col0, const tvec4<T>& col1, const tvec4<T>& col2, const tvec4<T>& col3) {
        col[0] = col0;
        col[1] = col1;
        col[2] = col2;
        col[3] = col3;
    }

    void operator=(const tmat3<T>& m)
    {
        (*this) = tmat4<T>(1.0f);
        col[0][0] = m[0][0]; col[0][1] = m[0][1]; col[0][2] = m[0][2];
        col[1][0] = m[1][0]; col[1][1] = m[1][1]; col[1][2] = m[1][2];
        col[2][0] = m[2][0]; col[2][1] = m[2][1]; col[2][2] = m[2][2];
    }

    void operator=(const tmat3x4<T>& m);
    
    tvec4<T> operator[](const int &i) const {
        return col[i];
    }
    tvec4<T>& operator[](const int &i){
        return col[i];
    }

    bool is_valid() const {
        for (int i = 0; i < 4; ++i) {
            if (!col[i].is_valid()) {
                return false;
            }
        }
        return true;
    }
private:
    tvec4<T> col[4];
};

template<typename T>
struct tmat3x4 {
    tmat3x4(){}
    explicit tmat3x4(T f) {
        col[0].x = f;
        col[1].y = f;
        col[2].z = f;
    }
    void operator=(const tmat3<T>& m) {
        (*this) = tmat3x4<T>(1.0f);
        col[0][0] = m[0][0]; col[0][1] = m[0][1]; col[0][2] = m[0][2];
        col[1][0] = m[1][0]; col[1][1] = m[1][1]; col[1][2] = m[1][2];
        col[2][0] = m[2][0]; col[2][1] = m[2][1]; col[2][2] = m[2][2];
    }
    void operator=(const tmat4<T>& m) {
        (*this) = tmat3x4<T>(1.0f);
        col[0][0] = m[0][0]; col[0][1] = m[0][1]; col[0][2] = m[0][2];
        col[1][0] = m[1][0]; col[1][1] = m[1][1]; col[1][2] = m[1][2];
        col[2][0] = m[2][0]; col[2][1] = m[2][1]; col[2][2] = m[2][2];
    }

    tvec4<T> operator[](const int &i) const {
        return col[i];
    }
    tvec4<T>& operator[](const int &i) {
        return col[i];
    }

    bool is_valid() const {
        for (int i = 0; i < 4; ++i) {
            if (!col[i].is_valid()) {
                return false;
            }
        }
        return true;
    }
private:
    tvec3<T> col[4];
};

template<typename T>
void tmat4<T>::operator=(const tmat3x4<T>& m) {
    col[0][0] = m[0][0]; col[0][1] = m[0][1]; col[0][2] = m[0][2];
    col[1][0] = m[1][0]; col[1][1] = m[1][1]; col[1][2] = m[1][2];
    col[2][0] = m[2][0]; col[2][1] = m[2][1]; col[2][2] = m[2][2];
}

template<typename T>
struct tray
{
	tray()
	{}
	tray(float x, float y, float z, float dx, float dy, float dz, float length = 1.f)
	: origin(tvec3<T>(x, y, z)), direction(dx, dy, dz), direction_inverse(1.0f / dx, 1.0f / dy, 1.0f / dz), length(length)
	{}
	tray(const tvec3<T>& origin, const tvec3<T>& direction, float length = 1.f)
	: origin(origin), direction(direction), direction_inverse(1.0f / direction.x, 1.0f / direction.y, 1.0f / direction.z),
        length(length)
	{}

    void update_inverse() {
        direction_inverse = tvec3<T>(
            1.0f / direction.x,
            1.0f / direction.y,
            1.0f / direction.z
        );
    }
	
	tvec3<T> origin;
	tvec3<T> direction;
    tvec3<T> direction_inverse; // for divisions
    float length = 1.f; // direction is always normalized, length stored separately
};

template<typename T>
struct taabb 
{
    taabb(){}
    taabb(const tvec3<T>& from, const tvec3<T>& to)
    : from(from), to(to) {}
    taabb(float ax, float ay, float az, float bx, float by, float bz)
    : from(ax, ay, az), to(bx, by, bz) {}

    tvec3<T> from;
    tvec3<T> to;
};

inline float lerp(float a, float b, float x);

template<typename T>
struct trect {
    trect() {}
    trect(const tvec2<T>& min, const tvec2<T>& max)
    : min(min), max(max) {}
    trect(float minx, float miny, float maxx, float maxy)
    : min(minx, miny), max(maxx, maxy) {}

    tvec2<T> min;
    tvec2<T> max;

    tvec2<T> center() const {
        return tvec2<T>(
            lerp(min.x, max.x, .5f),
            lerp(min.y, max.y, .5f)
        );
    }
    tvec2<T> size() const {
        return tvec2<T>(
            max.x - min.x,
            max.y - min.y
        );
    }
};

typedef tvec2<float> vec2;
typedef tvec2<int32_t> ivec2;
typedef tvec2<double> dvec2;

typedef tvec3<float> vec3;
typedef tvec3<int32_t> ivec3;
typedef tvec3<double> dvec3;

typedef tvec4<float> vec4;
typedef tvec4<int32_t> ivec4;
typedef tvec4<double> dvec4;

typedef tquat<float> quat;
typedef tquat<double> dquat;

typedef tmat3<float> mat3;
typedef tmat3<double> dmat3;

typedef tmat4<float> mat4;
typedef tmat4<double> dmat4;

typedef tray<float> ray;
typedef tray<double> dray;

typedef taabb<float> aabb;
typedef taabb<double> daabb;

typedef trect<float> rect;
typedef trect<double> drect;

// ====== Constants ======

const quat quat_identity = quat(.0f, .0f, .0f, 1.f);
const dquat dquat_identity = dquat(.0, .0, .0, 1.);

// ====== Functions ======

template<typename T>
T _min(T a, T b)
{
    if (a < b)
        return a;
    else
        return b;
}

template<typename T>
T _max(T a, T b)
{
    if (a > b)
        return a;
    else
        return b;
}

template<typename T>
inline tvec2<T> operator+(const tvec2<T>& a, const tvec2<T>& b){
    return tvec2<T>(a.x + b.x, a.y + b.y);
}
template<typename T>
inline tvec3<T> operator+(const tvec3<T>& a, const tvec3<T>& b){
    return tvec3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}
template<typename T>
inline tvec4<T> operator+(const tvec4<T>& a, const tvec4<T>& b){
    return tvec4<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}
template<typename T>
inline tquat<T> operator+(const tquat<T>& a, const tquat<T>& b) {
    return tquat<T>(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

template<typename T>
inline tvec2<T> operator+=(tvec2<T>& a, const tvec2<T>& b){
    return a = a + b;
}
template<typename T>
inline tvec3<T> operator+=(tvec3<T>& a, const tvec3<T>& b){
    return a = a + b;
}
template<typename T>
inline tvec4<T> operator+=(tvec4<T>& a, const tvec4<T>& b){
    return a = a + b;
}
template<typename T>
inline tquat<T> operator+=(tquat<T>& a, const tquat<T>& b) {
    return a = a + b;
}

template<typename T>
inline tvec2<T> operator-(const tvec2<T>& a, const tvec2<T>& b){
    return tvec2<T>(a.x - b.x, a.y - b.y);
}
template<typename T>
inline tvec3<T> operator-(const tvec3<T>& a, const tvec3<T>& b){
    return tvec3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}
template<typename T>
inline tvec4<T> operator-(const tvec4<T>& a, const tvec4<T>& b){
    return tvec4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}
template<typename T>
inline tquat<T> operator-(const tquat<T>& a, const tquat<T>& b){
    return tvec4<T>(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

template<typename T>
inline tvec2<T> operator-=(tvec2<T>& a, const tvec2<T>& b){
    return a = a - b;
}
template<typename T>
inline tvec3<T> operator-=(tvec3<T>& a, const tvec3<T>& b){
    return a = a - b;
}
template<typename T>
inline tvec4<T> operator-=(tvec4<T>& a, const tvec4<T>& b){
    return a = a - b;
}
template<typename T>
inline tquat<T> operator-=(tquat<T>& a, const tquat<T>& b) {
    return a = a - b;
}

template<typename T>
inline tvec2<T> operator-(const tvec2<T>& v){
    return tvec2<T>(-v.x, -v.y);
}
template<typename T>
inline tvec3<T> operator-(const tvec3<T>& v){
    return tvec3<T>(-v.x, -v.y, -v.z);
}
template<typename T>
inline tvec4<T> operator-(const tvec4<T>& v){
    return tvec4<T>(-v.x, -v.y, -v.z, -v.w);
}
template<typename T>
inline tquat<T> operator-(const tquat<T>& v) {
    return tquat<T>(-v.x, -v.y, -v.z, -v.w);
}

template<typename T>
inline tvec2<T> operator*(const tvec2<T>& a, const tvec2<T>& f) {
    return tvec2<T>(a.x * f.x, a.y * f.y);
}
template<typename T, typename M>
inline tvec2<T> operator*(const tvec2<T>& a, const M& f){
    return tvec2<T>(a.x * f, a.y * f);
}
template<typename T, typename M>
inline tvec2<T> operator*(const M& f, const tvec2<T>& a){
    return tvec2<T>(a.x * f, a.y * f);
}
template<typename T>
inline tvec3<T> operator*(const tvec3<T>& a, const tvec3<T>& f) {
    return tvec3<T>(a.x * f.x, a.y * f.y, a.z * f.z);
}
template<typename T, typename M>
inline tvec3<T> operator*(const tvec3<T>& a, const M& f){
    return tvec3<T>(a.x * f, a.y * f, a.z * f);
}
template<typename T, typename M>
inline tvec3<T> operator*(const M& f, const tvec3<T>& a){
    return tvec3<T>(a.x * f, a.y * f, a.z * f);
}
template<typename T>
inline tvec4<T> operator*(const tvec4<T>& a, const tvec4<T>& f) {
    return tvec4<T>(a.x * f.x, a.y * f.y, a.z * f.z, a.w * f.w);
}
template<typename T, typename M>
inline tvec4<T> operator*(const tvec4<T>& a, const M& f){
    return tvec4<T>(a.x * f, a.y * f, a.z * f, a.w * f);
}
template<typename T, typename M>
inline tvec4<T> operator*(const M& f, const tvec4<T>& a){
    return tvec4<T>(a.x * f, a.y * f, a.z * f, a.w * f);
}

template<typename T, typename M>
inline tvec2<T> operator*=(tvec2<T>& a, const M& f){
    return a = a * f;
}
template<typename T, typename M>
inline tvec3<T> operator*=(tvec3<T>& a, const M& f){
    return a = a * f;
}
template<typename T, typename M>
inline tvec4<T> operator*=(tvec4<T>& a, const M& f){
    return a = a * f;
}

template<typename T, typename M>
inline tvec2<T> operator/(const tvec2<T>& a, const M& f){
    return tvec2<T>(a.x / f, a.y / f);
}
template<typename T, typename M>
inline tvec2<T> operator/(const tvec2<T>& a, const tvec2<M>& b){
    return tvec2<T>(a.x / b.x, a.y / b.y);
}
template<typename T, typename M>
inline tvec3<T> operator/(const tvec3<T>& a, const M& f){
    return tvec3<T>(a.x / f, a.y / f, a.z / f);
}
template<typename T, typename M>
inline tvec4<T> operator/(const tvec4<T>& a, const M& f){
    return tvec4<T>(a.x / f, a.y / f, a.z / f, a.w / f);
}
template<typename T, typename M>
inline tquat<T> operator/(const tquat<T>& a, const M& f) {
    return tquat<T>(a.x / f, a.y / f, a.z / f, a.w / f);
}

template<typename T, typename M>
inline tvec2<T>& operator/=(tvec2<T>& a, const M& f){
    a.x /= f;
    a.y /= f;
    return a;
}
template<typename T, typename M>
inline tvec3<T>& operator/=(tvec3<T>& a, const M& f){
    a.x /= f;
    a.y /= f;
    a.z /= f;
    return a;
}
template<typename T, typename M>
inline tvec4<T>& operator/=(tvec4<T>& a, const M& f){
    a.x /= f;
    a.y /= f;
    a.z /= f;
    a.w /= f;
    return a;
}
template<typename T, typename M>
inline tquat<T>& operator/=(tquat<T>& a, const M& f) {
    a.x /= f;
    a.y /= f;
    a.z /= f;
    a.w /= f;
    return a;
}

template<typename T, typename T2>
inline bool operator>(const tvec2<T>& a, const tvec2<T2>& b) {
    return a.x > b.x && a.y > b.y;
}
template<typename T, typename T2>
inline bool operator>(const tvec3<T>& a, const tvec3<T2>& b) {
    return a.x > b.x && a.y > b.y && a.z > b.z;
}

template<typename T, typename T2>
inline bool operator<(const tvec2<T>& a, const tvec2<T2>& b) {
    return a.x < b.x && a.y < b.y;
}
template<typename T, typename T2>
inline bool operator<(const tvec3<T>& a, const tvec3<T2>& b) {
    return a.x < b.x && a.y < b.y && a.z < b.z;
}

template<typename T, typename T2>
inline bool operator>=(const tvec2<T>& a, const tvec2<T2>& b) {
    return a.x >= b.x && a.y >= b.y;
}
template<typename T, typename T2>
inline bool operator>=(const tvec3<T>& a, const tvec3<T2>& b) {
    return a.x >= b.x && a.y >= b.y && a.z >= b.z;
}

template<typename T, typename T2>
inline bool operator<=(const tvec2<T>& a, const tvec2<T2>& b) {
    return a.x <= b.x && a.y <= b.y;
}
template<typename T, typename T2>
inline bool operator<=(const tvec3<T>& a, const tvec3<T2>& b) {
    return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}

inline float qrsqrt(const float &n)
{
    long i;
    float x2, y;
    const float threehalves = 1.5f;
    x2 = n * 0.5f;
    y = n;
    i = *(long*)&y;
    i = 0x5f3759df - (i >> 1);
    y = *(float*)&i;
    y = y * (threehalves - (x2 * y * y));
    //y = y * (threehalves - (x2 * y * y));
    return y;
}

inline float sqrt(const float &n)
{
    return ::sqrtf(n);
    //return n * qrsqrt(n);
}
inline float pow2(float n) {
    return n * n;
}
inline float pow2_sign(float n) {
    return fabsf(n) * n;
}

inline float sign(float v) {
    return v < .0f ? -1.f : 1.f;
}

template<typename T>
inline T length(const tvec2<T>& v) { return v.length(); }
template<typename T>
inline T length(const tvec3<T>& v) { return v.length(); }
template<typename T>
inline T length(const tvec4<T>& v) { return v.length(); }
template<typename T>
inline T length(const tquat<T>& q) { return sqrt(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w); }

template<typename T>
inline tvec2<T> normalize(const tvec2<T>& v) 
{
    T l = length(v);
    if(l == T())
        return v;
    return v / l;
}

template<typename T>
inline tvec3<T> normalize(const tvec3<T>& v) 
{
    T l = length(v);
    if(l == T())
        return v;
    return v / l;
}

template<typename T>
inline tvec4<T> normalize(const tvec4<T>& v) 
{
    T l = length(v);
    if(l == T())
        return v;
    return v / l;
}

template<typename T>
inline tquat<T> normalize(const tquat<T>& a) {
    if (length(a) == 0.0f)
        return a;
    return a / length(a);
}

template<typename T>
inline T dot(const tvec2<T>& a, const tvec2<T>& b)
{
    return a.x * b.x + a.y * b.y;
}
template<typename T>
inline T dot(const tvec3<T>& a, const tvec3<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
template<typename T>
inline T dot(const tvec4<T>& a, const tvec4<T>& b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
template<typename T>
inline T dot(const tquat<T>& a, const tquat<T>& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

template<typename T>
inline tvec3<T> cross(const tvec3<T>& a, const tvec3<T>& b)
{
    return tvec3<T>(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

template<typename T>
inline tmat4<T> operator+(const tmat4<T>& m0, const tmat4<T>& m1) {
    tmat4<T> m;
    for (int i = 0; i < 4; ++i)
        m[i] = m0[i] + m1[i];
    return m;
}
template<typename T>
inline tmat3<T> operator*(const tmat3<T>& m0, const tmat3<T>& m1) {
    tmat3<T> m;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k)
                m[i][j] += m0[k][j] * m1[i][k];
    return m;
}
template<typename T>
inline tmat4<T> operator*(const tmat4<T>& m0, const tmat4<T>& m1) {
    tmat4<T> m;
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k)
                m[i][j] += m0[k][j] * m1[i][k];
    return m;
}

template<typename T>
inline tvec4<T> operator*(const tmat4<T>& m, const tvec4<T>& v)
{
    tvec4<T> r;
    for (int i = 0; i < 4; ++i)
        for (int k = 0; k < 4; ++k)
            r[i] += m[k][i] * v[k];
    return r;
}

template<typename T>
inline void mul_position(const tmat4<T>& m, const tvec3<T>& v, tvec3<T>& out) {
    out[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0];
    out[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1];
    out[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2];
}
template<typename T>
inline void mul_normal(const tmat4<T>& m, const tvec3<T>& v, tvec3<T>& out) {
    out[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2];
    out[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2];
    out[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2];
}
template<typename T>
inline void mul_position_add_weighted(const tmat4<T>& m, const tvec3<T>& v, float weight, tvec3<T>& out) {
    out[0] += (m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0]) * weight;
    out[1] += (m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1]) * weight;
    out[2] += (m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2]) * weight;
}
template<typename T>
inline void mul_normal_add_weighted(const tmat4<T>& m, const tvec3<T>& v, float weight, tvec3<T>& out) {
    out[0] += (m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2]) * weight;
    out[1] += (m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2]) * weight;
    out[2] += (m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2]) * weight;
}

//Taking vec3 as vec4 and assuming v.w is zero
//so it transforms as a direction vector
template<typename T>
inline tvec3<T> operator*(const tmat4<T> &m, const tvec3<T> &v)
{
    tvec3<T> r;
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k)
            r[i] += m[k][i] * v[k];
    return r;
}
template<typename T>
inline tvec3<T> operator*(const tmat3<T> &m, const tvec3<T> &v)
{
    tvec3<T> r;
    for (int i = 0; i < 3; ++i)
        for (int k = 0; k < 3; ++k)
            r[i] += m[k][i] * v[k];
    return r;
}
template<typename T>
inline tmat3<T> transpose(const tmat3<T>& m)
{
    tmat3<T> r(1.0f);
    for (unsigned i = 0; i < 3; ++i)
        for (unsigned j = 0; j < 3; ++j)
            r[i][j] = m[j][i];
    return r;
}
template<typename T>
inline tmat4<T> transpose(const tmat4<T>& m)
{
    tmat4<T> r(1.0f);
    for (unsigned i = 0; i < 4; ++i)
        for (unsigned j = 0; j < 4; ++j)
            r[i][j] = m[j][i];
    return r;
}
template<typename T>
inline tmat4<T> scale(const tmat4<T>& m, const tvec3<T>& v)
{
    tmat4<T> r = m;
    r[0] *= v[0];
    r[1] *= v[1];
    r[2] *= v[2];
    return r;
}
template<typename T>
inline tmat4<T> translate(const tmat4<T> &m, const tvec3<T> &v)
{
    tmat4<T> r = m;
    r[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return r;
}

template<typename T>
inline tmat4<T> inverse(const tmat4<T> &mat)
{
    const T* m;
    m = (T*)&mat;
    T det;
    int i;
    tmat4<T> inverse(1.0f);
    T* inv = (T*)&inverse;
    
    inv[0] = m[5]  * m[10] * m[15] - 
             m[5]  * m[11] * m[14] - 
             m[9]  * m[6]  * m[15] + 
             m[9]  * m[7]  * m[14] +
             m[13] * m[6]  * m[11] - 
             m[13] * m[7]  * m[10];

    inv[4] = -m[4]  * m[10] * m[15] + 
              m[4]  * m[11] * m[14] + 
              m[8]  * m[6]  * m[15] - 
              m[8]  * m[7]  * m[14] - 
              m[12] * m[6]  * m[11] + 
              m[12] * m[7]  * m[10];

    inv[8] = m[4]  * m[9] * m[15] - 
             m[4]  * m[11] * m[13] - 
             m[8]  * m[5] * m[15] + 
             m[8]  * m[7] * m[13] + 
             m[12] * m[5] * m[11] - 
             m[12] * m[7] * m[9];

    inv[12] = -m[4]  * m[9] * m[14] + 
               m[4]  * m[10] * m[13] +
               m[8]  * m[5] * m[14] - 
               m[8]  * m[6] * m[13] - 
               m[12] * m[5] * m[10] + 
               m[12] * m[6] * m[9];

    inv[1] = -m[1]  * m[10] * m[15] + 
              m[1]  * m[11] * m[14] + 
              m[9]  * m[2] * m[15] - 
              m[9]  * m[3] * m[14] - 
              m[13] * m[2] * m[11] + 
              m[13] * m[3] * m[10];

    inv[5] = m[0]  * m[10] * m[15] - 
             m[0]  * m[11] * m[14] - 
             m[8]  * m[2] * m[15] + 
             m[8]  * m[3] * m[14] + 
             m[12] * m[2] * m[11] - 
             m[12] * m[3] * m[10];

    inv[9] = -m[0]  * m[9] * m[15] + 
              m[0]  * m[11] * m[13] + 
              m[8]  * m[1] * m[15] - 
              m[8]  * m[3] * m[13] - 
              m[12] * m[1] * m[11] + 
              m[12] * m[3] * m[9];

    inv[13] = m[0]  * m[9] * m[14] - 
              m[0]  * m[10] * m[13] - 
              m[8]  * m[1] * m[14] + 
              m[8]  * m[2] * m[13] + 
              m[12] * m[1] * m[10] - 
              m[12] * m[2] * m[9];

    inv[2] = m[1]  * m[6] * m[15] - 
             m[1]  * m[7] * m[14] - 
             m[5]  * m[2] * m[15] + 
             m[5]  * m[3] * m[14] + 
             m[13] * m[2] * m[7] - 
             m[13] * m[3] * m[6];

    inv[6] = -m[0]  * m[6] * m[15] + 
              m[0]  * m[7] * m[14] + 
              m[4]  * m[2] * m[15] - 
              m[4]  * m[3] * m[14] - 
              m[12] * m[2] * m[7] + 
              m[12] * m[3] * m[6];

    inv[10] = m[0]  * m[5] * m[15] - 
              m[0]  * m[7] * m[13] - 
              m[4]  * m[1] * m[15] + 
              m[4]  * m[3] * m[13] + 
              m[12] * m[1] * m[7] - 
              m[12] * m[3] * m[5];

    inv[14] = -m[0]  * m[5] * m[14] + 
               m[0]  * m[6] * m[13] + 
               m[4]  * m[1] * m[14] - 
               m[4]  * m[2] * m[13] - 
               m[12] * m[1] * m[6] + 
               m[12] * m[2] * m[5];

    inv[3] = -m[1] * m[6] * m[11] + 
              m[1] * m[7] * m[10] + 
              m[5] * m[2] * m[11] - 
              m[5] * m[3] * m[10] - 
              m[9] * m[2] * m[7] + 
              m[9] * m[3] * m[6];

    inv[7] = m[0] * m[6] * m[11] - 
             m[0] * m[7] * m[10] - 
             m[4] * m[2] * m[11] + 
             m[4] * m[3] * m[10] + 
             m[8] * m[2] * m[7] - 
             m[8] * m[3] * m[6];

    inv[11] = -m[0] * m[5] * m[11] + 
               m[0] * m[7] * m[9] + 
               m[4] * m[1] * m[11] - 
               m[4] * m[3] * m[9] - 
               m[8] * m[1] * m[7] + 
               m[8] * m[3] * m[5];

    inv[15] = m[0] * m[5] * m[10] - 
              m[0] * m[6] * m[9] - 
              m[4] * m[1] * m[10] + 
              m[4] * m[2] * m[9] + 
              m[8] * m[1] * m[6] - 
              m[8] * m[2] * m[5];

    det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

    //assert(det != 0);

    det = 1.0f / det;

    for (i = 0; i < 16; i++)
        inv[i] = inv[i] * det;

    return inverse;
}

template<typename T>
inline tquat<T> operator*(const tquat<T>& q0, const tquat<T>& q1)
{
    return normalize(tquat<T>((q0.w * q1.x + q1.w * q0.x) + (q0.y * q1.z - q1.y * q0.z),
        (q0.w * q1.y + q1.w * q0.y) + (q1.x * q0.z - q0.x * q1.z), //Inverted, since y axis rotation is inverted
        (q0.w * q1.z + q1.w * q0.z) + (q0.x * q1.y - q1.x * q0.y),
        (q1.w * q0.w) - (q1.x * q0.x) - (q1.y * q0.y) - (q1.z * q0.z)));
}
template<typename T>
inline tquat<T> operator*=(tquat<T>& q0, const tquat<T>& q1)
{
    return q0 = q0 * q1;
}

template<typename T, typename M>
inline tquat<T> operator*(const tquat<T>& a, const M& f) {
    return tquat<T>(a.x * f, a.y * f, a.z * f, a.w * f);
}
template<typename T, typename M>
inline tquat<T> operator*=(tquat<T>& a, const M& f) {
    return a = a * f;
}
template<typename T>
inline tquat<T> angle_axis(float a, const tvec3<T>& axis)
{
    float s = sinf(a * 0.5f);
    return normalize(tquat<T>(axis.x * s, axis.y * s, axis.z * s, cosf(a*0.5f)));
}
template<typename T>
inline tquat<T> inverse(const tquat<T>& q)
{
    tquat<T> i;
    float d = dot(q, q);
    i.x = -q.x / d;
    i.y = -q.y / d;
    i.z = -q.z / d;
    i.w = q.w / d;
    i = normalize(i);
    return i;
}
template<typename T>
inline tmat3<T> to_mat3(const tquat<T>& q)
{
    tmat3<T> m;
    float x = q.x;
    float y = q.y;
    float z = q.z;
    float w = q.w;
    float _2x2 = 2 * x * x;
    float _2y2 = 2 * y * y;
    float _2z2 = 2 * z * z;
    float _2xy = 2 * x * y;
    float _2wz = 2 * w * z;
    float _2xz = 2 * x * z;
    float _2wy = 2 * w * y;
    float _2wx = 2 * w * x;
    float _2yz = 2 * y * z;


    m[0][0] = 1 - _2y2 - _2z2;
    m[0][1] = _2xy + _2wz;
    m[0][2] = _2xz - _2wy;
    m[1][0] = _2xy - _2wz;
    m[1][1] = 1 - _2x2 - _2z2;
    m[1][2] = _2yz + _2wx;
    m[2][0] = _2xz + _2wy;
    m[2][1] = _2yz - _2wx;
    m[2][2] = 1 - _2x2 - _2y2;

    return m;
}
template<typename T>
inline tmat3<T> to_mat3(const tmat4<T>& m4)
{
    tmat3<T> m3;
    for (unsigned i = 0; i < 3; ++i)
        m3[i] = m4[i];
    return m3;
}
template<typename T>
inline tmat3<T> to_orient_mat3(const tmat4<T>& m)
{
    tmat3<T> mt = to_mat3(m);

    for (unsigned i = 0; i < 3; ++i)
    {
        tvec3<T> v3 = mt[i];
        v3 = normalize(v3);
        mt[i] = v3;
    }

    return mt;
}
template<typename T>
inline tmat4<T> to_mat4(const tquat<T>& q)
{
    tmat4<T> m(1.0f);
    m = to_mat3(q);
    return m;
}
template<typename T>
inline tmat4<T> to_mat4(const tmat3<T>& m3) {
    tmat4<T> m(1.0f);
    m[0] = gfxm::vec4(m3[0].x, m3[0].y, m3[0].z, .0f);
    m[1] = gfxm::vec4(m3[1].x, m3[1].y, m3[1].z, .0f);
    m[2] = gfxm::vec4(m3[2].x, m3[2].y, m3[2].z, .0f);
    return m;
}
template<typename T>
inline tquat<T> euler_to_quat(const tvec3<T>& euler)
{
    tquat<T> qx = angle_axis(euler.x, tvec3<T>(1.0f, 0.0f, 0.0f));
    tquat<T> qy = angle_axis(euler.y, tvec3<T>(0.0f, 1.0f, 0.0f));
    tquat<T> qz = angle_axis(euler.z, tvec3<T>(0.0f, 0.0f, 1.0f));
    return normalize(qz * qy * qx);
}
template<typename T>
T roll(const tquat<T>& q) {
    return static_cast<T>(
        atan2(
            static_cast<T>(2) * (q.x * q.y + q.w * q.z),
            q.w * q.w + q.x * q.x - q.y * q.y - q.z * q.z
        )
    );
}
template<typename T>
T pitch(const tquat<T>& q) {
    T const y = static_cast<T>(2) * (q.y * q.z + q.w * q.x);
    T const x = q.w * q.w - q.x * q.x - q.y * q.y + q.z * q.z;

    if(std::abs(x) <= std::numeric_limits<float>::epsilon() &&
        std::abs(y) <= std::numeric_limits<float>::epsilon())
    {
        return static_cast<T>(static_cast<T>(2) * atan2(q.x, q.w));    
    }

    return static_cast<T>(atan2(y, x));
}
template<typename T>
T yaw(const tquat<T>& q) {
    float clamp(float f, float a, float b);

    return asin(
        clamp(
            static_cast<T>(-2) * (q.x * q.z - q.w * q.y),
            static_cast<T>(-1),
            static_cast<T>(1)
        )
    );
}
template<typename T>
inline tvec3<T> to_euler(const tquat<T>& q)
{
    return tvec3<T>(pitch(q), yaw(q), roll(q));
}
template<typename T>
inline tquat<T> to_quat(const tvec3<T>& euler) {
    return euler_to_quat(euler);
}

template<typename T>
inline tvec2<T> to_euler_xy(const tmat4<T>& m) {
    tvec2<T> out;
    gfxm::vec3 dir = m[2];
    gfxm::vec3 diry = dir;
    float y = diry.y;
    diry.y = .0f;
    float z = diry.length();
    diry = gfxm::normalize(diry);
    gfxm::vec2 dir2 = gfxm::normalize(gfxm::vec2(y, z));
    out.x = -atan2f(dir2.x, dir2.y);
    out.y = atan2f(diry.x, diry.z);
    return out;
}
/*
template<typename T>
inline tquat<T> to_quat(tmat3<T>& m)
{
    tquat<T> q(0.0f, 0.0f, 0.0f, 1.0f);

    T& m00 = m[0][0]; T& m01 = m[0][1]; T& m02 = m[0][2];
    T& m10 = m[1][0]; T& m11 = m[1][1]; T& m12 = m[1][2];
    T& m20 = m[2][0]; T& m21 = m[2][1]; T& m22 = m[2][2];
    T& x = q.x;
    T& y = q.y;
    T& z = q.z;
    T& w = q.w;

    // Use the Graphics Gems code, from 
    // ftp://ftp.cis.upenn.edu/pub/graphics/shoemake/quatut.ps.Z
    T t = m00 + m11 + m22;
    // we protect the division by s by ensuring that s>=1
    if (t >= 0) { // by w
        T s = sqrt(t + (T)1);
        w = (T)0.5 * s;
        s = (T)0.5 / s;                 
        x = (m21 - m12) * s;
        y = (m02 - m20) * s;
        z = (m10 - m01) * s;
    } else if ((m00 > m11) && (m00 > m22)) { // by x
        T s = sqrt(1 + m00 - m11 - m22); 
        x = s * (T)0.5; 
        s = (T)0.5 / s;
        y = (m10 + m01) * s;
        z = (m02 + m20) * s;
        w = (m21 - m12) * s;
    } else if (m11 > m22) { // by y
        T s = sqrt(1 + m11 - m00 - m22); 
        y = s * (T)0.5; 
        s = (T)0.5 / s;
        x = (m10 + m01) * s;
        z = (m21 + m12) * s;
        w = (m02 - m20) * s;
    } else { // by z
        T s = sqrt(1 + m22 - m00 - m11); 
        z = s * (T)0.5; 
        s = (T)0.5 / s;
        x = (m02 + m20) * s;
        y = (m21 + m12) * s;
        w = (m10 - m01) * s;
    }

    return normalize(q);
}
*/
template<typename T>
inline tquat<T> to_quat(const tmat3<T>& m)
{
    tquat<T> q(0.0f, 0.0f, 0.0f, 1.0f);

    float t;
    if (m[2][2] < 0)
    {
        if (m[0][0] > m[1][1])
        {
            t = 1.0f + m[0][0] - m[1][1] - m[2][2];
            q = tquat<T>(t, m[0][1] + m[1][0], m[2][0] + m[0][2], m[1][2] - m[2][1]);
        }
        else
        {
            t = 1 - m[0][0] + m[1][1] - m[2][2];
            q = tquat<T>(m[0][1] + m[1][0], t, m[1][2] + m[2][1], m[2][0] - m[0][2]);
        }
    }
    else
    {
        if (m[0][0] < -m[1][1])
        {
            t = 1 - m[0][0] - m[1][1] + m[2][2];
            q = tquat<T>(m[2][0] + m[0][2], m[1][2] + m[2][1], t, m[0][1] - m[1][0]);
        }
        else
        {
            t = 1 + m[0][0] + m[1][1] + m[2][2];
            q = tquat<T>(m[1][2] - m[2][1], m[2][0] - m[0][2], m[0][1] - m[1][0], t);
        }
    }

    q *= 0.5f / sqrt(t);

    return normalize(q);
}
template<typename T>
inline tmat4<T>& perspective(tmat4<T>& m, float fov, float aspect, float znear, float zfar)
{
    //assert(aspect != 0.0f);
    //assert(zfar != znear);

    float tanHalfFovy = tanf(fov / 2.0f);

    m = tmat4<T>(0);
    m[0][0] = 1.0f / (aspect * tanHalfFovy);
    m[1][1] = 1.0f / (tanHalfFovy);
    m[2][2] = -(zfar + znear) / (zfar - znear);
    m[2][3] = -1.0f;
    m[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
    return m;
}

template<typename T>
inline tmat4<T> perspective(T fov, T aspect, T znear, T zfar)
{
    float tanHalfFovy = tanf(fov / 2.0f);
    tmat4<T> m = tmat4<T>(0);
    m[0][0] = 1.0f / (aspect * tanHalfFovy);
    m[1][1] = 1.0f / (tanHalfFovy);
    m[2][2] = -(zfar + znear) / (zfar - znear);
    m[2][3] = -1.0f;
    m[3][2] = -(2.0f * zfar * znear) / (zfar - znear);
    return m;
}

template<typename T>
inline tmat4<T>& ortho(tmat4<T>& m, float left, float right, float bottom, float top, float znear, float zfar)
{
    m = tmat4<T>(1.0f);
    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -2.0f / (zfar - znear);
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
    m[3][2] = -(zfar + znear) / (zfar - znear);
    return m;
}

template<typename T>
inline tmat4<T> ortho(T left, T right, T bottom, T top, T znear, T zfar)
{
    tmat4<T> m = tmat4<T>(1.0f);
    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -2.0f / (zfar - znear);
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
    m[3][2] = -(zfar + znear) / (zfar - znear);
    return m;
}

//

inline float clamp(float f, float a, float b)
{
    f = f < a ? a : (f > b ? b : f);
    return f;
}

inline float lerp(float a, float b, float x)
{
    return (a * (1.0f - x)) + (b * x);
}

inline float smoothstep(float a, float b, float x)
{
    x = x * x * (3 - 2 * x);
    return lerp(a, b, x);
}

template<typename T>
inline tvec2<T> lerp(const tvec2<T>& a, const tvec2<T>& b, float x)
{
    return tvec2<T>(lerp(a.x, b.x, x), lerp(a.y, b.y, x));
}

template<typename T>
inline tvec3<T> lerp(const tvec3<T>& a, const tvec3<T>& b, float x)
{
    return tvec3<T>(lerp(a.x, b.x, x), lerp(a.y, b.y, x), lerp(a.z, b.z, x));
}

template<typename T>
inline tvec3<T> slerp(const tvec3<T>& a, const tvec3<T>& b, float x) {
    float d = dot(a, b);
    d = clamp(d, -1.f, 1.f);
    float theta = acos(d) * x;
    tvec3<T> rel = normalize(b - a * d);
    return ((a * cos(theta)) + (rel * sin(theta)));
}

template<typename T>
inline tvec4<T> lerp(const tvec4<T>& a, const tvec4<T>& b, float x)
{
    return tvec4<T>(lerp(a.x, b.x, x), lerp(a.y, b.y, x), lerp(a.z, b.z, x), lerp(a.w, b.w, x));
}

template<typename T>
inline tquat<T> lerp(const tquat<T>& a, const tquat<T>& b, float x)
{
    return normalize(a * (1.0f - x) + b * x);
}

template<typename T>
inline tquat<T> slerp(const tquat<T>& a, const tquat<T>& b, float x)
{
    tquat<T> r = tquat<T>(0.0f, 0.0f, 0.0f, 1.0f);
    float d = dot(a, b);

    if (d < 0.0f)
    {
        d = -d;
        r = -b;
    }
    else
    {
        r = b;
    }

    if (d < 0.9995f)
    {
        //float angle = acosf(d);
        //return (a * sinf(angle * (1.0f - x)) + r * sinf(angle * x)) / sinf(angle);
        float theta_0 = acosf(d);        // theta_0 = angle between input vectors
        float theta = theta_0 * x;          // theta = angle between v0 and result
        float sin_theta = sinf(theta);     // compute this value only once
        float sin_theta_0 = sinf(theta_0); // compute this value only once

        float s0 = cosf(theta) - d * sin_theta / sin_theta_0;  // == sin(theta_0 - theta) / sin(theta_0)
        float s1 = sin_theta / sin_theta_0;

        return normalize((a * s0) + (r * s1));
    }
    else
    {
        return lerp(a, r, x);
    }
}

// Transform class (opengl coordinate system)

class transform
{
public:
    transform()
        : t(0.0f, 0.0f, 0.0f),
        r(0.0f, 0.0f, 0.0f, 1.0f),
        s(1.0f, 1.0f, 1.0f)
    {

    }
    void translate(float x, float y, float z)
    {
        translate(vec3(x, y, z));
    }
    void translate(const vec3& vec)
    {
        t = t + vec;
    }

    void rotate(float angle, float axisX, float axisY, float axisZ)
    {
	    rotate(angle, vec3(axisX, axisY, axisZ));
    }
    void rotate(float angle, const vec3& axis)
    {
	    rotate(angle_axis(angle, axis));
    }
    void rotate(const quat& q)
    {
	    r = normalize(q * r);
    }

    void position(float x, float y, float z)
    {
        position(vec3(x, y, z));
    }
    void position(const vec3& position)
    {
        t = position;
    }

    void rotation(float x, float y, float z)
    {
        r = euler_to_quat(vec3(x, y, z));
    }
    void rotation(float x, float y, float z, float w)
    {
        rotation(quat(x, y, z, w));
    }
    void rotation(const quat& rotation)
    {
        r = rotation;
    }

    void scale(float x)
    {
        scale(vec3(x, x, x));
    }
    void scale(float x, float y, float z)
    {
        scale(vec3(x, y, z));
    }
    void scale(const vec3& scale)
    {
        s = scale;
    }

    vec3 position()
    {
        return t;
    }
    quat rotation()
    {
        return r;
    }
    vec3 scale()
    {
        return s;
    }

    vec3 right(){ return matrix()[0]; }
    vec3 up(){ return matrix()[1]; }
    vec3 back() { return matrix()[2]; }
    vec3 left() { return -right(); }
    vec3 down() { return -up(); }
    vec3 forward() { return -back(); }

    void set_transform(mat4& mat)
    {
        t = vec3(mat[3].x, mat[3].y, mat[3].z);
        mat3 rotMat = to_orient_mat3(mat);
        r = to_quat(rotMat);
        vec3 right = mat[0];
        vec3 up = mat[1];
        vec3 back = mat[2];
        s = vec3(length(right), length(up), length(back));
    }

    mat4 matrix()
    {
        return 
            ::gfxm::translate(mat4(1.0f), t) * 
            to_mat4(r) * 
            ::gfxm::scale(mat4(1.0f), s);
    }

    void look_at(const vec3& target, const vec3& forward, const vec3& up = vec3(0.0f, 1.0f, 0.0f), float f = 1.0f)
    {
        f = _max(-1.0f, _min(f, 1.0f));

        mat4 mat = matrix();
        vec3 pos = mat[3];

        vec3 newFwdUnit = normalize(target - pos);
        vec3 rotAxis = normalize(cross(forward, newFwdUnit));

        quat q;
        float d = dot(forward, newFwdUnit);

        const float eps = 0.01f;
        if (fabs(d + 1.0f) <= eps)
        {
            q = angle_axis(pi * f, up);
        }/*
         else if(fabs(d - 1.0f) <= eps)
         {
         q = gfxm::quat(0.0f, 0.0f, 0.0f, 1.0f);
         }*/
        else
        {
            float rotAngle = acosf(_max(-1.0f, _min(d, 1.0f))) * f;
            q = angle_axis(rotAngle, rotAxis);
        }

        rotate(q);
    }
private:
    vec3 t;
    quat r;
    vec3 s;
	mat4 m;
};

inline float radian(float deg) {
    return deg * (pi / 180.0f);
}
inline float degrees(float rad) {
    return rad * (180.0f / pi);
}

template<typename T>
inline tmat4<T> lookAt(const tvec3<T>& eye, const tvec3<T>& center, const tvec3<T>& up) {
    tvec3<T> const f(normalize(center - eye));
    tvec3<T> const s(normalize(cross(f, up)));
    tvec3<T> const u(cross(s, f));

    tmat4<T> res(1);
    res[0][0] = s.x;
    res[1][0] = s.y;
    res[2][0] = s.z;
    res[0][1] = u.x;
    res[1][1] = u.y;
    res[2][1] = u.z;
    res[0][2] = -f.x;
    res[1][2] = -f.y;
    res[2][2] = -f.z;
    res[3][0] =-dot(s, eye);
    res[3][1] =-dot(u, eye);
    res[3][2] = dot(f, eye);
    return res;
}

inline gfxm::vec2 world_to_screen(
    const gfxm::vec3& pt_world,
    const gfxm::mat4& projection,
    const gfxm::mat4& view,
    float vp_width, float vp_height
) {
    gfxm::vec4 pt_screen4 = projection * view * gfxm::vec4(pt_world, 1.f);
    if (pt_screen4.w != .0f) {
        pt_screen4 /= pt_screen4.w;
    }
    gfxm::vec2 pt_screen2 = (gfxm::vec2(pt_screen4.x, -pt_screen4.y) + gfxm::vec2(1.f, 1.f)) * .5f;
    pt_screen2 *= gfxm::vec2(vp_width, vp_height);
    return gfxm::vec2(pt_screen2.x, vp_height - pt_screen2.y);
}

template<typename T>
inline tray<T> ray_viewport_to_world(
    const tvec2<T>& viewport_size,
    const tvec2<T>& cursor_pos,
    const tmat4<T>& projection,
    const tmat4<T>& view
) {
    gfxm::tvec2<T> scr_spc_pos(
        cursor_pos.x / viewport_size.x * 2.0f - 1.0f,
        cursor_pos.y / viewport_size.y * 2.0f - 1.0f
    );
    gfxm::tvec4<T> wpos4_origin(
        scr_spc_pos.x,
        scr_spc_pos.y,
        -1.0f, 1.0f
    );
    gfxm::tvec4<T> wpos4(
        scr_spc_pos.x,
        scr_spc_pos.y,
        1.0f, 1.0f
    );
    //gfxm::tvec3<T> ray_origin = inverse(view) * tvec4<T>(0.0f, 0.0f, 0.0f, 1.0f);
    wpos4_origin = inverse(projection * view) * wpos4_origin;
    gfxm::tvec3<T> ray_origin = gfxm::tvec3<T>(wpos4_origin.x, wpos4_origin.y, wpos4_origin.z) / wpos4_origin.w;
    
    wpos4 = inverse(projection * view) * wpos4;
    tvec3<T> wpos = gfxm::tvec3<T>(wpos4.x, wpos4.y, wpos4.z) / wpos4.w;
    tvec3<T> direction = normalize(
        wpos - ray_origin
    );

    return tray<T>(ray_origin, direction, (wpos - ray_origin).length());
}

template<typename T>
inline tvec3<T> screenToWorldPlaneXY(
    const tvec2<T>& scr_pos,
    const tvec2<T>& scr_sz,
    const tmat4<T>& proj,
    const tmat4<T>& view
) {
    gfxm::tvec2<T> scr_spc_pos(
        scr_pos.x / scr_sz.x * 2.0f - 1.0f,
        scr_pos.y / scr_sz.y * 2.0f - 1.0f
    );
    gfxm::tvec4<T> wpos4(
        scr_spc_pos.x, 
        scr_spc_pos.y,
        -1.0f, 1.0f
    );
    gfxm::tvec3<T> ray_origin = inverse(view) * tvec4<T>(0.0f, 0.0f, 0.0f, 1.0f);
    wpos4 = inverse(proj * view) * wpos4;
    tvec3<T> wpos = gfxm::tvec3<T>(wpos4.x, wpos4.y, wpos4.z) / wpos4.w;
    tvec3<T> ray = normalize(
        wpos - ray_origin
    );

    tvec3<T> v_to_target = ray;
    float ym = -ray_origin.y;
    float xc = ray.x / ray.y;
    float zc = ray.z / ray.y;
    v_to_target = tvec3<T>(
        xc * ym, ym, zc * ym
    );

    return ray_origin + v_to_target;
}

inline gfxm::aabb aabb_transform(const gfxm::aabb& box, const gfxm::mat4& transform) {
    gfxm::vec3 points[8] = {
        box.from,
        gfxm::vec3(box.from.x, box.from.y, box.to.z),
        gfxm::vec3(box.from.x, box.to.y, box.from.z),
        gfxm::vec3(box.to.x, box.from.y, box.from.z),
        box.to,
        gfxm::vec3(box.to.x, box.to.y, box.from.z),
        gfxm::vec3(box.to.x, box.from.y, box.to.z),
        gfxm::vec3(box.from.x, box.to.y, box.to.z)
    };
    for(int i = 0; i < 8; ++i) {
        points[i] = transform * gfxm::vec4(points[i], 1.0f);
    }
    gfxm::aabb aabb;
    aabb.from = points[0];
    aabb.to   = points[0];
    for(int i = 1; i < 8; ++i) {
        aabb.from = gfxm::vec3(
            gfxm::_min(aabb.from.x, points[i].x),
            gfxm::_min(aabb.from.y, points[i].y),
            gfxm::_min(aabb.from.z, points[i].z)
        );
        aabb.to = gfxm::vec3(
            gfxm::_max(aabb.to.x, points[i].x),
            gfxm::_max(aabb.to.y, points[i].y),
            gfxm::_max(aabb.to.z, points[i].z)
        );
    }
    return aabb;
}

inline void expand_aabb(gfxm::aabb& box, const gfxm::vec3& pt) {
    if(pt.x < box.from.x) box.from.x = pt.x;
    if(pt.y < box.from.y) box.from.y = pt.y;
    if(pt.z < box.from.z) box.from.z = pt.z;
    if(pt.x > box.to.x) box.to.x = pt.x;
    if(pt.y > box.to.y) box.to.y = pt.y;
    if(pt.z > box.to.z) box.to.z = pt.z;
}

inline gfxm::aabb aabb_union(const gfxm::aabb& a, const gfxm::aabb& b) {
    gfxm::aabb u;
    u.from.x = _min(a.from.x, b.from.x);
    u.from.y = _min(a.from.y, b.from.y);
    u.from.z = _min(a.from.z, b.from.z);
    u.to.x = _max(a.to.x, b.to.x);
    u.to.y = _max(a.to.y, b.to.y);
    u.to.z = _max(a.to.z, b.to.z);
    return u;
}

inline gfxm::aabb aabb_grow(const gfxm::aabb& a, float amount) {
    gfxm::aabb aabb;
    aabb = a;
    aabb.from.x -= amount;
    aabb.from.y -= amount;
    aabb.from.z -= amount;
    aabb.to.x += amount;
    aabb.to.y += amount;
    aabb.to.z += amount;
    return aabb;
}

inline float volume(const gfxm::aabb& box) {
    return (box.to.x - box.from.x) * (box.to.y - box.from.y) * (box.to.z - box.from.z);
}

inline void expand(gfxm::rect& rc, float val) {
    rc.min -= gfxm::vec2(val, val);
    rc.max += gfxm::vec2(val, val);
}
inline void expand(gfxm::rect& rc_out, const gfxm::rect& other) {
    rc_out.min.x = _min(rc_out.min.x, other.min.x);
    rc_out.min.y = _min(rc_out.min.y, other.min.y);
    rc_out.max.x = _max(rc_out.max.x, other.max.x);
    rc_out.max.y = _max(rc_out.max.y, other.max.y);
}

inline gfxm::vec2 rect_size(const gfxm::rect& rc) {
    return gfxm::vec2(rc.max.x - rc.min.x, rc.max.y - rc.min.y);
}

inline bool point_in_rect(const gfxm::rect& rc, const gfxm::vec2& pt) {
    return (pt.x >= rc.min.x && pt.x <= rc.max.x
        && pt.y >= rc.min.y && pt.y <= rc.max.y);
}

inline bool point_in_aabb(const gfxm::aabb& box, const gfxm::vec3& pt) {
    return pt >= box.from && pt <= box.to;
}
inline bool aabb_in_aabb(const gfxm::aabb& a, const gfxm::aabb& enclosing) {
    return a.from >= enclosing.from && a.to <= enclosing.to;
}

struct plane {
    gfxm::vec3 normal;
    float d = .0f;
};

enum FRUSTUM_SIDE {
    FRUSTUM_PLANE_NX,
    FRUSTUM_PLANE_PX,
    FRUSTUM_PLANE_NY,
    FRUSTUM_PLANE_PY,
    FRUSTUM_PLANE_NZ,
    FRUSTUM_PLANE_PZ
};

struct frustum {
    // -X +X -Y +Y -Z +Z
    gfxm::plane planes[6];
};

inline frustum make_frustum(const mat4& proj) {
    frustum f;

    // Left plane
    f.planes[0].normal.x = proj[0][3] + proj[0][0];
    f.planes[0].normal.y = proj[1][3] + proj[1][0];
    f.planes[0].normal.z = proj[2][3] + proj[2][0];
    f.planes[0].d = proj[3][3] + proj[3][0];

    // Right plane
    f.planes[1].normal.x = proj[0][3] - proj[0][0];
    f.planes[1].normal.y = proj[1][3] - proj[1][0];
    f.planes[1].normal.z = proj[2][3] - proj[2][0];
    f.planes[1].d = proj[3][3] - proj[3][0];

    // Bottom plane
    f.planes[2].normal.x = proj[0][3] + proj[0][1];
    f.planes[2].normal.y = proj[1][3] + proj[1][1];
    f.planes[2].normal.z = proj[2][3] + proj[2][1];
    f.planes[2].d = proj[3][3] + proj[3][1];

    // Top plane
    f.planes[3].normal.x = proj[0][3] - proj[0][1];
    f.planes[3].normal.y = proj[1][3] - proj[1][1];
    f.planes[3].normal.z = proj[2][3] - proj[2][1];
    f.planes[3].d = proj[3][3] - proj[3][1];

    float a = proj[2][2];
    float b = proj[3][2];
    float znear = b / (a - 1.f);
    float zfar = b / (a + 1.f);

    // Far plane
    f.planes[4].normal.x = proj[0][3] - proj[0][2];
    f.planes[4].normal.y = proj[1][3] - proj[1][2];
    f.planes[4].normal.z = proj[2][3] - proj[2][2];
    f.planes[4].d = -zfar;// proj[3][3] + proj[3][2];

    // Near plane
    f.planes[5].normal.x = proj[0][3] + proj[0][2];
    f.planes[5].normal.y = proj[1][3] + proj[1][2];
    f.planes[5].normal.z = proj[2][3] + proj[2][2];
    f.planes[5].d = znear;//proj[3][3] - proj[3][2];

    for (int i = 0; i < 6; ++i) {
        f.planes[i].normal = gfxm::normalize(f.planes[i].normal);
    }

    return f;
}

inline frustum make_frustum(const mat4& proj, const mat4& view) {
    frustum f = make_frustum(proj);

    const gfxm::mat4 inv_view = gfxm::inverse(view);
    for (int i = 0; i < 6; ++i) {
        auto& plane = f.planes[i];
        const gfxm::vec3 old_normal = plane.normal;
        plane.normal = inv_view * gfxm::vec4(plane.normal, .0f);
        plane.d = gfxm::dot(plane.normal, gfxm::vec3(inv_view * gfxm::vec4(old_normal * plane.d, 1.f)));
    }

    return f;
}

inline bool intersect_frustum_aabb(const frustum& f, const aabb& aabb) {
    const gfxm::vec3 vertices[8] = {
        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.from.z),
        gfxm::vec3(aabb.from.x, aabb.from.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.from.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.from.y, aabb.from.z),
        gfxm::vec3(aabb.from.x, aabb.to.y, aabb.from.z),
        gfxm::vec3(aabb.from.x, aabb.to.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.to.y, aabb.to.z),
        gfxm::vec3(aabb.to.x, aabb.to.y, aabb.from.z),
    };

    for (int i = 0; i < 6; ++i) {
        const auto& plane = f.planes[i];
        float max_d = gfxm::dot(plane.normal, vertices[0]);
        for (int j = 1; j < 8; ++j) {
            const float d = gfxm::dot(plane.normal, vertices[j]);
            max_d = gfxm::_max(max_d, d);
        }

        if (max_d > plane.d) {
            continue;
        }

        return false;
    }

    return true;
}

inline bool make_portal_frustum(
    const gfxm::mat4& proj,
    const gfxm::mat4& view,
    const gfxm::rect& ndc_bounds,
    const gfxm::mat4& portal_transform,
    const gfxm::vec2& portal_half_extents,
    gfxm::frustum& out_frustum,
    gfxm::rect& out_ndc_bounds
) {
    const gfxm::vec3 N_portal = gfxm::normalize(gfxm::vec3(portal_transform[2]));
    const gfxm::vec3 P_portal = gfxm::vec3(portal_transform[3]);
    const gfxm::mat4 inv_view = gfxm::inverse(view);
    const gfxm::vec3 P_camera = gfxm::vec3(inv_view[3]);

    {
        float d = gfxm::dot(N_portal, P_camera - P_portal);
        if (d <= .0f) {
            return false;
        }
    }
        
    gfxm::vec4 points_ndc[4] = {
        proj * view * portal_transform * gfxm::vec4(-portal_half_extents.x, -portal_half_extents.y, .0f, 1.f),
        proj * view * portal_transform * gfxm::vec4(portal_half_extents.x, -portal_half_extents.y, .0f, 1.f),
        proj * view * portal_transform * gfxm::vec4(portal_half_extents.x, portal_half_extents.y, .0f, 1.f),
        proj * view * portal_transform * gfxm::vec4(-portal_half_extents.x, portal_half_extents.y, .0f, 1.f)
    };
    points_ndc[0] /= points_ndc[0].w;
    points_ndc[1] /= points_ndc[1].w;
    points_ndc[2] /= points_ndc[2].w;
    points_ndc[3] /= points_ndc[3].w;

    gfxm::rect aabb_ndc;
    aabb_ndc.min.x = gfxm::_max(ndc_bounds.min.x, points_ndc[0].x);
    aabb_ndc.min.y = gfxm::_max(ndc_bounds.min.y, points_ndc[0].y);
    aabb_ndc.max.x = gfxm::_min(ndc_bounds.max.x, points_ndc[0].x);
    aabb_ndc.max.y = gfxm::_min(ndc_bounds.max.y, points_ndc[0].y);
    for (int i = 1; i < 4; ++i) {
        aabb_ndc.min.x = gfxm::_min(aabb_ndc.min.x, gfxm::_max(ndc_bounds.min.x, points_ndc[i].x));
        aabb_ndc.min.y = gfxm::_min(aabb_ndc.min.y, gfxm::_max(ndc_bounds.min.y, points_ndc[i].y));
        aabb_ndc.max.x = gfxm::_max(aabb_ndc.max.x, gfxm::_min(ndc_bounds.max.x, points_ndc[i].x));
        aabb_ndc.max.y = gfxm::_max(aabb_ndc.max.y, gfxm::_min(ndc_bounds.max.y, points_ndc[i].y));
    }

    if (aabb_ndc.min.x >= aabb_ndc.max.x || aabb_ndc.min.y >= aabb_ndc.max.y) {
        return false;
    }
        
    // Frustum plane normals for the full view
    gfxm::vec3 Nnx = gfxm::normalize(gfxm::vec3(proj[0][3] + proj[0][0], proj[1][3] + proj[1][0], proj[2][3] + proj[2][0]));
    gfxm::vec3 Npx = gfxm::normalize(gfxm::vec3(proj[0][3] - proj[0][0], proj[1][3] - proj[1][0], proj[2][3] - proj[2][0]));
    gfxm::vec3 Nny = gfxm::normalize(gfxm::vec3(proj[0][3] + proj[0][1], proj[1][3] + proj[1][1], proj[2][3] + proj[2][1]));
    gfxm::vec3 Npy = gfxm::normalize(gfxm::vec3(proj[0][3] - proj[0][1], proj[1][3] - proj[1][1], proj[2][3] - proj[2][1]));

    const gfxm::mat4 inv_proj = gfxm::inverse(proj);
    gfxm::vec4 points_view[2] = {
        inv_proj * gfxm::vec4(aabb_ndc.min.x, aabb_ndc.min.y, .0f, 1.f),
        inv_proj * gfxm::vec4(aabb_ndc.max.x, aabb_ndc.max.y, .0f, 1.f)
    };
    points_view[0] /= points_view[0].w;
    points_view[1] /= points_view[1].w;

    // Left plane
    out_frustum.planes[0].normal = gfxm::lerp(Nnx, -Npx, (aabb_ndc.min.x + 1.f) * .5f);
    out_frustum.planes[0].d = gfxm::dot(out_frustum.planes[0].normal, gfxm::vec3(points_view[0]));
    // Right plane
    out_frustum.planes[1].normal = gfxm::lerp(-Nnx, Npx, (aabb_ndc.max.x + 1.f) * .5f);
    out_frustum.planes[1].d = gfxm::dot(out_frustum.planes[1].normal, gfxm::vec3(points_view[1]));
    // Bottom plane
    out_frustum.planes[2].normal = gfxm::lerp(Nny, -Npy, (aabb_ndc.min.y + 1.f) * .5f);
    out_frustum.planes[2].d = gfxm::dot(out_frustum.planes[2].normal, gfxm::vec3(points_view[0]));
    // Top plane
    out_frustum.planes[3].normal = gfxm::lerp(-Nny, Npy, (aabb_ndc.max.y + 1.f) * .5f);
    out_frustum.planes[3].d = gfxm::dot(out_frustum.planes[3].normal, gfxm::vec3(points_view[1]));

    const float a = proj[2][2];
    const float b = proj[3][2];
    const float znear = b / (a - 1.f);
    const float zfar = b / (a + 1.f);

    // Far plane
    out_frustum.planes[4].normal.x = proj[0][3] - proj[0][2];
    out_frustum.planes[4].normal.y = proj[1][3] - proj[1][2];
    out_frustum.planes[4].normal.z = proj[2][3] - proj[2][2];
    out_frustum.planes[4].d = -zfar;

    // Near plane
    out_frustum.planes[5].normal.x = proj[0][3] + proj[0][2];
    out_frustum.planes[5].normal.y = proj[1][3] + proj[1][2];
    out_frustum.planes[5].normal.z = proj[2][3] + proj[2][2];
    out_frustum.planes[5].d = znear;

    for (int i = 0; i < 6; ++i) {
        out_frustum.planes[i].normal = gfxm::normalize(out_frustum.planes[i].normal);
    }

    for (int i = 0; i < 6; ++i) {
        auto& plane = out_frustum.planes[i];
        const gfxm::vec3 old_normal = plane.normal;
        plane.normal = inv_view * gfxm::vec4(plane.normal, .0f);
        plane.d = gfxm::dot(plane.normal, gfxm::vec3(inv_view * gfxm::vec4(old_normal * plane.d, 1.f)));
    }

    out_ndc_bounds = aabb_ndc;

    return true;
}


inline uint32_t make_rgba32(float R, float G, float B, float A) {
    uint32_t rc = _min(255, int(255 * R));
    uint32_t gc = _min(255, int(255 * G));
    uint32_t bc = _min(255, int(255 * B));
    uint32_t ac = _min(255, int(255 * A));
    uint32_t C = 0;
    C |= rc;
    C |= gc << 8;
    C |= bc << 16;
    C |= ac << 24;
    return C;
}

inline gfxm::vec4 make_rgba4f(uint32_t rgba32) {
    gfxm::vec4 rgba;
    rgba.r = (rgba32 & (0xFF)) / 255.f;
    rgba.g = (rgba32 & (0xFF << 8)) / 255.f;
    rgba.b = (rgba32 & (0xFF << 16)) / 255.f;
    rgba.a = (rgba32 & (0xFF << 24)) / 255.f;
    return rgba;
}

inline uint32_t lerp_color(uint32_t color_a, uint32_t color_b, float t) {
    uint8_t t8 = static_cast<uint8_t>(t * 255);
    uint8_t inv_alpha = 255 - t8;

    uint32_t a1 = (color_a >> 24) & 0xff;
    uint32_t r1 = (color_a >> 16) & 0xff;
    uint32_t g1 = (color_a >> 8) & 0xff;
    uint32_t b1 = (color_a) & 0xff;

    uint32_t a2 = (color_b >> 24) & 0xff;
    uint32_t r2 = (color_b >> 16) & 0xff;
    uint32_t g2 = (color_b >> 8) & 0xff;
    uint32_t b2 = (color_b) & 0xff;

    uint32_t a = ((a1 * inv_alpha) + (a2 * t8)) / 255;
    uint32_t r = ((r1 * inv_alpha) + (r2 * t8)) / 255;
    uint32_t g = ((g1 * inv_alpha) + (g2 * t8)) / 255;
    uint32_t b = ((b1 * inv_alpha) + (b2 * t8)) / 255;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

inline gfxm::mat3 make_orientation_yz(const gfxm::vec3& axis_y, const gfxm::vec3& axis_z) {
    gfxm::mat3 m3(1.f);
    m3[2] = gfxm::normalize(axis_z);
    m3[0] = gfxm::normalize(gfxm::cross(axis_y, m3[2]));
    m3[1] = gfxm::normalize(gfxm::cross(m3[2], m3[0]));
    return m3;
}

inline gfxm::vec2 project_point_xy(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v) {
    gfxm::vec2 v2d;
    v2d.x = gfxm::dot(m[0], v - origin);
    v2d.y = gfxm::dot(m[1], v - origin);
    return v2d;
}
inline gfxm::vec2 project_point_xz(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v) {
    gfxm::vec2 v2d;
    v2d.x = gfxm::dot(m[0], v - origin);
    v2d.y = gfxm::dot(m[2], v - origin);
    return v2d;
}
inline gfxm::vec2 project_point_yz(const gfxm::mat3& m, const gfxm::vec3& origin, const gfxm::vec3& v) {
    gfxm::vec2 v2d;
    v2d.x = gfxm::dot(m[1], v - origin);
    v2d.y = gfxm::dot(m[2], v - origin);
    return v2d;
}

inline gfxm::vec3 unproject_point_xy(const gfxm::vec2& v2d, const gfxm::vec3& origin, const gfxm::vec3& axisX, const gfxm::vec3& axisY) {
    return origin + axisX * v2d.x + axisY * v2d.y;
}

}

#endif
