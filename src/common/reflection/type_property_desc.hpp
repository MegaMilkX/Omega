#pragma once

#include <string>
#include "type.hpp"


namespace rtti {

struct PropSnapshot;
class varying;
struct type_property_desc {
    type t;
    std::string name;
    bool writable = true;
    bool readable = true;
    
    std::function<varying(const MetaObject*)> fn_get_varying = nullptr;
    std::function<void*(const MetaObject*)> fn_get_ptr = nullptr;
    std::function<void(MetaObject*, void*)> fn_get_value = nullptr;
    std::function<void(MetaObject*, const void*)> fn_set = nullptr;
    std::function<MetaObject*(const MetaObject* object)> fn_as_meta_object = nullptr;
    //std::function<void(void*, void*)> fn_setter;
    //std::function<void(void*, void*)> fn_getter;

    std::function<void(const void*, nlohmann::json&)> fn_serialize_json = nullptr;
    std::function<void(void*, const nlohmann::json&)> fn_deserialize_json = nullptr;

    varying get_value(const MetaObject* object) const;

    template<typename T>
    T getValue(MetaObject* object) const {
        T value = T();        
        if (type_get<T>() != t) {
            assert(false);
            return value;
        }
        
        if (fn_get_value) {
            fn_get_value(object, &value);
        } else if(fn_get_ptr) {
            void* ptr = fn_get_ptr(object);
            value = *(T*)ptr;
        }
        return value;
    }
    template<typename T, typename std::enable_if<!std::is_pointer<T>::value, int>::value* = nullptr>
    void setValue(MetaObject* object, const T& value) const {
        if (type_get<T>() != t) {
            assert(false);
            return;
        }
        setValue(object, (void*)&value);
    }
    void setValue(MetaObject* object, void* value) const {
        if (fn_set) {
            fn_set(object, value);
        } else if(fn_get_ptr) {
            void* ptr = fn_get_ptr(object);
            memcpy(ptr, value, t.get_size());
        }
    }
};


}

