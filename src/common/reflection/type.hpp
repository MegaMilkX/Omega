#pragma once

#include "nlohmann/json.hpp"
#include "common.hpp"
#include "property.hpp"


class varying;
struct type_property_desc;
struct type_desc;
struct type {
    type_uid_t guid;

    type()
        : guid(0) {}
    type(type_uid_t guid)
        : guid(guid) {}

    size_t      get_size() const;
    const char* get_name() const;
    const type_desc* get_desc() const;

    bool is_valid() const;

    bool is_pointer() const;
    bool is_copy_constructible() const;

    bool is_derived_from(type other) const;

    int   prop_count() const;
    const type_property_desc* get_prop(int i);
    property get_property(int i);
    
    varying get_prop_value(const MetaObject* object, int prop_idx);

    template<typename O, typename T>
    void set_property(const char* name, O* object, const T& value);
    void set_property_unsafe(const char* name, MetaObject* object, void* value) const;

    void  construct(void* ptr);
    void  destruct(void* ptr);
    void* construct_new();
    void  destruct_delete(void* ptr);
    template<typename BASE_T>
    BASE_T* construct_new();
    void  copy_construct(void* ptr, const void* other);

    void serialize_json(nlohmann::json& j, const void* object) const;
    bool deserialize_json(const nlohmann::json& j, void* object) const;
    void serialize_json(const char* filename, const void* object);
    bool deserialize_json(const char* filename, void* object);

    void dbg_print();

    bool operator==(const type& other) const { return guid == other.guid; }
    bool operator!=(const type& other) const { return guid != other.guid; }
    bool operator<(const type& other) const { return guid < other.guid; }
    operator bool() const { return (*this) != type(0); }
};
template<>
struct std::hash<type> {
    size_t operator()(const type& t) const {
        return std::hash<type_uid_t>()(t.guid);
    }
};

