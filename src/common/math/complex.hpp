#pragma once

#include <math.h>

namespace gfxm {


template<typename T>
struct tcomplex {
    tcomplex()
        : real(0), imag(0) {}
    tcomplex(T real, T imag)
        : real(real), imag(imag) {}
    tcomplex(T real)
        : real(real), imag(0) {}

    tcomplex<T> conj() const {
        return tcomplex<T>(real, -imag);
    }
    float mag() const {
        return sqrtf(real * real + imag * imag);
    }
    float phase() const {
        return atan2f(imag, real);
    }
    tcomplex<T> normalized() const {
        return (*this) / mag();
    }

    T real;
    T imag;
};

template<typename COMPLEX_T, typename T>
inline tcomplex<COMPLEX_T> operator*(tcomplex<COMPLEX_T> c, T r) {
    return tcomplex<COMPLEX_T>(c.real * r, c.imag * r);
}
template<typename COMPLEX_T, typename T>
inline tcomplex<COMPLEX_T> operator*(T r, tcomplex<COMPLEX_T> c) {
    return tcomplex<COMPLEX_T>(c.real * r, c.imag * r);
}
template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T> operator*(tcomplex<COMPLEX_T> a, tcomplex<COMPLEX_T> b) {
    return tcomplex<COMPLEX_T>(a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real);
}
template<typename COMPLEX_T, typename T>
inline tcomplex<COMPLEX_T>& operator*=(tcomplex<COMPLEX_T>& a, T r){
    a.real *= r;
    a.imag *= r;
    return a;
}
template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T>& operator*=(tcomplex<COMPLEX_T>& a, tcomplex<COMPLEX_T> b) {
    tcomplex<COMPLEX_T> res(a.real * b.real - a.imag * b.imag, a.real * b.imag + a.imag * b.real);
    a = res;
    return a;
}

template<typename COMPLEX_T, typename T>
inline tcomplex<COMPLEX_T> operator/(tcomplex<COMPLEX_T> c, T r) {
    return tcomplex<COMPLEX_T>(c.real / r, c.imag / r);
}
template<typename COMPLEX_T, typename T>
inline tcomplex<COMPLEX_T>& operator/=(tcomplex<COMPLEX_T>& a, T r) {
    a.real /= r;
    a.imag /= r;
    return a;
}

template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T> operator+(tcomplex<COMPLEX_T> a, tcomplex<COMPLEX_T> b) {
    return tcomplex<COMPLEX_T>(a.real + b.real, a.imag + b.imag);
}
template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T>& operator+=(tcomplex<COMPLEX_T>& a, const tcomplex<COMPLEX_T> b) {
    a.real += b.real;
    a.imag += b.imag;
    return a;
}

template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T> operator-(tcomplex<COMPLEX_T> a, tcomplex<COMPLEX_T> b) {
    return tcomplex<COMPLEX_T>(a.real - b.real, a.imag - b.imag);
}
template<typename COMPLEX_T>
inline tcomplex<COMPLEX_T>& operator-=(tcomplex<COMPLEX_T>& a, const tcomplex<COMPLEX_T> b) {
    a.real -= b.real;
    a.imag -= b.imag;
    return a;
}

typedef tcomplex<float> complex;
typedef tcomplex<double> complexd;


}

