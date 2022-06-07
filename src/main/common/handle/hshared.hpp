#pragma once

#include "handle.hpp"
#include "log/log.hpp"

// TODO:
template<typename T>
class HUNIQUE;

// ====

template<typename T>
class HSHARED {
    Handle<T> handle;
    uint32_t* ref_count = 0;
public:
    HSHARED(Handle<T> h = 0)
    : handle(h), ref_count(new uint32_t(1)) {}
    HSHARED(HSHARED<T>& other)
    : handle(other.handle), ref_count(other.ref_count) {
        ++(*ref_count);
    }
    HSHARED(const HSHARED<T>& other)
    : handle(other.handle), ref_count(other.ref_count) {
        ++(*ref_count);
    }
    ~HSHARED() {
        --(*ref_count);
        if (*ref_count == 0) {
            HANDLE_MGR<T>::release(handle);
            delete ref_count;
        }
    }

    void reset(Handle<T> h = 0) {
        --(*ref_count);
        if (*ref_count == 0) {
            HANDLE_MGR<T>::release(handle);
            delete ref_count;
        }
        ref_count = new uint32_t(1);
        handle = h;
    }

    bool isValid() const { return HANDLE_MGR<T>::isValid(handle); }
    T*   get() { return HANDLE_MGR<T>::deref(handle); }
    const T* get() const { return HANDLE_MGR<T>::deref(handle); }
    Handle<T> getHandle() { return handle; }
    uint32_t  refCount() const { return *ref_count; }

    operator bool() const { return HANDLE_MGR<T>::isValid(handle); }
    T* operator->() { return HANDLE_MGR<T>::deref(handle); }
    const T* operator->() const { return HANDLE_MGR<T>::deref(handle); }
    T& operator*() { return *HANDLE_MGR<T>::deref(handle); }
    const T& operator*() const { return *HANDLE_MGR<T>::deref(handle); }
    HSHARED<T>& operator=(const HSHARED<T>& other) {
        if (ref_count) {
            --(*ref_count);
            if (*ref_count == 0) {
                HANDLE_MGR<T>::release(handle);
                delete ref_count;
            }
        }
        handle = other.handle;
        ref_count = other.ref_count;
        ++(*ref_count);
        return *this;
    }
};


template<typename T>
class HWEAK {
    Handle<T> handle;
public:
    HWEAK(Handle<T> h = 0)
    : handle(h) {}
    HWEAK(HSHARED<T>& other)
    : handle(other.getHandle()) {}
    HWEAK(const HSHARED<T>& other)
    : handle(other.getHandle()) {}
    ~HWEAK() {}

    bool isValid() const { return HANDLE_MGR<T>::isValid(handle); }
    operator bool() const { return isValid(); }
    T* operator->() { return HANDLE_MGR<T>::deref(handle); }
    const T* operator->() const { return HANDLE_MGR<T>::deref(handle); }
    T& operator*() { return *HANDLE_MGR<T>::deref(handle); }
    const T& operator*() const { return *HANDLE_MGR<T>::deref(handle); }
    HWEAK<T>& operator=(HSHARED<T>& other) { handle = other.getHandle(); }
};