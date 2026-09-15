#pragma once

#include "nlohmann/json.hpp"
#include "common.hpp"
#include "property.hpp"
#include "enumerator_desc.hpp"


class ResourceRefBase;
class Resource;

namespace rtti {


class varying;
struct type_property_desc;
struct type_desc;
struct type {
    type_id_t id;

    type()
        : id(0) {}
    type(type_id_t id)
        : id(id) {}

    size_t      get_size() const;
    const char* get_name() const;
    const type_desc* get_desc() const;

    bool is_valid() const;

    bool is_enum() const;
    bool is_pointer() const;
    bool is_wrapper() const;
    type get_wrapped_type() const;
    bool is_constructible() const;
    bool is_copy_constructible() const;

    bool is_derived_from(type other) const;

    int   enumerator_count() const;
    const enumerator_desc* get_enumerator(int i) const;
    void set_enum_value(void* object, long long val) const;
    long long get_enum_value(const void* object) const;

    int   prop_count() const;
    const type_property_desc* get_prop(int i);
    const type_property_desc* get_prop_desc(const std::string& prop_name);
    property get_property(int i);
    
    varying get_prop_value(const MetaObject* object, int prop_idx);

    template<typename O, typename T>
    void set_property(const char* name, O* object, const T& value);
    void set_property_unsafe(const char* name, MetaObject* object, void* value) const;

    void  construct(void* ptr) const;
    void  destruct(void* ptr) const;
    void* construct_new() const;
    Resource* construct_as_resource() const;
    void  destruct_delete(void* ptr) const;
    template<typename BASE_T>
    BASE_T* construct_new() const;
    void  copy_construct(void* ptr, const void* other) const;

    ResourceRefBase* as_resource_ref_base(void* object) const;

    void serialize_json(nlohmann::json& j, const void* object) const;
    bool deserialize_json(const nlohmann::json& j, void* object) const;
    void serialize_json(const char* filename, const void* object);
    bool deserialize_json(const char* filename, void* object);

    void dbg_print();

    bool operator==(const type& other) const { return id == other.id; }
    bool operator!=(const type& other) const { return id != other.id; }
    bool operator<(const type& other) const { return id < other.id; }
    operator bool() const { return (*this) != type(0); }
};


}


template<>
struct std::hash<rtti::type> {
    size_t operator()(const rtti::type& t) const {
        return std::hash<rtti::type_id_t>()(t.id);
    }
};

