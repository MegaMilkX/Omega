#pragma once

#include <vector>
#include "type.hpp"


namespace rtti {


class varying {
    std::vector<unsigned char> buffer;
    type t = type(0);

public:
    varying() {}
    varying(const varying& other) {
        clear();
        if (other.get_type().is_pointer()) {
            t = other.t;
            buffer = other.buffer;
        } else if(other.get_type().is_copy_constructible()) {
            t = other.t;
            buffer.resize(t.get_size());
            t.copy_construct(buffer.data(), other.buffer.data());
        }
    }
    varying(varying&& other) noexcept {
        clear();
        t = other.t;
        other.t = type(0);
        buffer = std::move(other.buffer);
    }

    varying& operator=(const varying& other) {
        clear();
        if (other.get_type().is_pointer()) {
            t = other.t;
            buffer = other.buffer;
        } else if(other.get_type().is_copy_constructible()) {
            t = other.t;
            buffer.resize(t.get_size());
            t.copy_construct(buffer.data(), other.buffer.data());
        }
        return *this;
    }
    varying& operator=(varying&& other) noexcept {
        clear();
        t = other.t;
        other.t = type(0);
        buffer = std::move(other.buffer);
        return *this;
    }

    ~varying() {
        clear();
    }

    static varying make(type t) {
        varying var;
        var.t = t;
        var.buffer.resize(t.get_size());
        t.construct(var.buffer.data());
        return var;
    }

    template<typename T>
    static varying make(const T& value) {
        auto t = type_get<T>();
        if (!t.is_copy_constructible()) {
            assert(false);
            return varying();
        }
        varying var;
        var.t = t;
        var.buffer.resize(sizeof(T));
        t.copy_construct(var.buffer.data(), &value);
        return var;
    }

    void clear() {
        if (t == type(0)) {
            return;
        }
        if (!t.is_pointer()) {
            t.destruct(buffer.data());
        }
        buffer.clear();
    }

    const void* data() const {
        return buffer.data();
    }

    type get_type() const { return t; }

    template<typename T>
    const T* get() const {
        if (type_get<T>() != t) {
            return nullptr;
        }
        return static_cast<const T*>((const void*)buffer.data());
    }
    template<typename T>
    T* get() {
        if (type_get<T>() != t) {
            return nullptr;
        }
        return static_cast<T*>((void*)buffer.data());
    }

    void to_json(nlohmann::json& j) const {
        t.serialize_json(j, buffer.data());
    }
    bool from_json(const nlohmann::json& j) {
        if(!t.is_valid()) return false;
        return t.deserialize_json(j, buffer.data());
    }

    bool set(type t, void* src) {
        if (!t.is_copy_constructible()) {
            return false;
        }
        buffer.resize(t.get_size());
        t.copy_construct(buffer.data(), src);
        this->t = t;
        return true;
    }

    template<typename T>
    std::enable_if_t<!std::is_pointer<T>::value, void> set(const T& value) {
        clear();

        t = type_get<unqualified_type<T>>();
        buffer.resize(t.get_size());
        t.construct(buffer.data());
    }
    template<typename T>
    std::enable_if_t<std::is_pointer<T>::value, void> set(T pointer) {
        clear();
        
        t = type_get<T>();
        buffer.resize(sizeof(void*));
        (*(void**)buffer.data()) = (void*)pointer;
    }
};


}

