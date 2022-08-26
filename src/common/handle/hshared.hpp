#pragma once

#include "handle.hpp"
#include "log/log.hpp"

// TODO:
template<typename T>
class HUNIQUE;

// ====

class HSHARED_BASE {
public:
    virtual ~HSHARED_BASE() {}
};
template<typename T>
class HSHARED : public HSHARED_BASE {
    Handle<T> handle{ 0 };
    uint32_t* ref_count = 0;
public:
    HSHARED(Handle<T> h = 0UL)
    : handle(h), ref_count(new uint32_t(1)) {}
    HSHARED(const HSHARED<T>& other)
    : handle(other.handle), ref_count(other.ref_count) {
        ++(*ref_count);
    }
    ~HSHARED() {
        --(*ref_count);
        if (*ref_count == 0) {
            if (HANDLE_MGR<T>::isValid(handle)) {
                HANDLE_MGR<T>::release(handle);
            }
            delete ref_count;
        }
    }

    void reset(Handle<T> h = 0) {
        --(*ref_count);
        if (*ref_count == 0) {
            if (HANDLE_MGR<T>::isValid(handle)) {
                HANDLE_MGR<T>::release(handle);
            }
            delete ref_count;
        }
        ref_count = new uint32_t(1);
        handle = h;
    }
    void reset_acquire() {
        reset(HANDLE_MGR<T>::acquire());
    }

    bool isValid() const { return HANDLE_MGR<T>::isValid(handle); }
    T*   get() { return HANDLE_MGR<T>::deref(handle); }
    const T* get() const { return HANDLE_MGR<T>::deref(handle); }
    Handle<T> getHandle() { return handle; }
    uint32_t  refCount() const { return *ref_count; }
    const std::string& getReferenceName() const {
        return HANDLE_MGR<T>::getReferenceName(handle);
    }
    void setReferenceName(const char* name) {
        HANDLE_MGR<T>::setReferenceName(handle, name);
    }

    operator bool() const { return HANDLE_MGR<T>::isValid(handle); }
    T* operator->() { return HANDLE_MGR<T>::deref(handle); }
    const T* operator->() const { return HANDLE_MGR<T>::deref(handle); }
    T& operator*() { return *HANDLE_MGR<T>::deref(handle); }
    const T& operator*() const { return *HANDLE_MGR<T>::deref(handle); }
    HSHARED<T>& operator=(const HSHARED<T>& other) {
        assert(ref_count);
        if (ref_count) {
            --(*ref_count);
            if (*ref_count == 0) {
                if (HANDLE_MGR<T>::isValid(handle)) {
                    HANDLE_MGR<T>::release(handle);
                }
                delete ref_count;
            }
        }
        handle = other.handle;
        ref_count = other.ref_count;
        ++(*ref_count);
        return *this;
    }

    bool serializeJson(const char* fname, bool link_handle_to_file = true) {
        type_get<T>().serialize_json(fname, get());
        if (link_handle_to_file) {
            setReferenceName(fname);
        }
        return true;
    }
};

// Introducing an alias to distinguish actual resource handles
// from non-resource handles
// TODO: Separate resource handle system
template<typename T>
using RHSHARED = HSHARED<T>;


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