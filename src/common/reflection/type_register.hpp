#pragma once

#include <string>
#include <vector>
#include <set>
#include "nlohmann/json.hpp"
#include "type.hpp"
#include "meta_object.hpp"
#include "type_property_desc.hpp"


namespace rtti {


template<class T>
struct GET_MEMBER_TYPE;

template<class C, class M>
struct GET_MEMBER_TYPE<M C::*> {
    using type = M;
};


template<typename T> struct ARGUMENT_CHECKER;

template<typename C, typename R, typename FirstArg, typename... Args>
struct ARGUMENT_CHECKER<R(C::*)(FirstArg, Args...)> {
    using ARG_TYPE = FirstArg;
    constexpr static int arg_count = 1 + sizeof...(Args);
};

template<typename C, typename R>
struct ARGUMENT_CHECKER<R(C::*)()> {
    constexpr static int arg_count = 0;
};

template<typename C, typename R>
struct ARGUMENT_CHECKER<R(C::*)() const> {
    constexpr static int arg_count = 0;
};

template<typename T>
concept has_own_get_type = requires {
    requires std::same_as<decltype(&T::get_type), type(T::*)()const>;
};

template<typename T>
class type_register {
    std::string name;
    std::set<type_desc::parent_info> parents;
    std::vector<type_property_desc> properties;
    void(*pfn_custom_serialize_json)(nlohmann::json&, const void*) = 0;
    void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*) = 0;
public:
    static_assert(
        !std::is_base_of_v<MetaObject, T> || has_own_get_type<T>,
        "Forgot TYPE_ENABLE() in T's body"
    );

    type_register(const char* name)
    : name(name) {
        // TODO
    }
    ~type_register() {
        auto desc = get_type_desc(type_get<T>());
        desc->name = name;
        desc->parent_types = parents;
        for (const auto& p : parents) {
            get_type_desc(p.parent_type)->derived_types.insert(type_get<T>());
        }
        for (int i = 0; i < properties.size(); ++i) {
            desc->properties.push_back(properties[i]);
        }
        desc->pfn_custom_serialize_json = pfn_custom_serialize_json;
        desc->pfn_custom_deserialize_json = pfn_custom_deserialize_json;

        {
            auto& map = get_type_name_map();
            map[name] = type_get<T>();
        }
    }
    template<typename PARENT_T>
    type_register<T>& parent() {
        static_assert(!std::is_same_v<PARENT_T, T>, "type_register: T can't be a parent of itself");
        static_assert(std::is_base_of_v<PARENT_T, T>, "type_register: T must derive from PARENT_T");
        /*
        ptrdiff_t poffs = reinterpret_cast<const char*>(
                static_cast<const PARENT_T*>(reinterpret_cast<const T*>(0x1000))
            ) - reinterpret_cast<const char*>(0x1000);
        */
        parents.insert(type_desc::parent_info{
            .parent_type = type_get<PARENT_T>(),
            .pfn_static_upcast = [](void* derived) -> void* {
                return static_cast<PARENT_T*>(static_cast<T*>(derived));
            }
        });
        return *this;
    }

    template<
        typename MEMBER_T,
        std::enable_if_t<std::is_member_object_pointer<MEMBER_T>::value>* = nullptr
    >
    type_register<T>& prop(const char* name, MEMBER_T member) {
        static_assert(std::is_base_of_v<MetaObject, T>, "T must inherit MetaObject to register properties");
        
        using MemberType = GET_MEMBER_TYPE<MEMBER_T>::type;

        type_property_desc prop_desc;
        prop_desc.name = name;
        prop_desc.t = type_get<MemberType>();
        prop_desc.fn_get_varying = [member](const MetaObject* object)->varying {
            return varying::make(((T*)object)->*member);
        };
        prop_desc.fn_get_ptr = [member](const MetaObject* object)->void* {
            return &(((T*)object)->*member);
        };
        prop_desc.fn_get_value = nullptr;
        
        prop_desc.fn_set = [member](MetaObject* object, const void* value) {
            // TODO: Does not compile for unique_ptr
            // figure it out!
            (((T*)object)->*member) = (*(MemberType*)value);
        };

        prop_desc.fn_serialize_json = [member](const void* object, nlohmann::json& j) {
            type_get<MemberType>().serialize_json(j, &(((T*)object)->*member));
        };
        prop_desc.fn_deserialize_json = [member](void* object, const nlohmann::json& j) {
            type_get<MemberType>().deserialize_json(j, &(((T*)object)->*member));
        };
        properties.push_back(prop_desc);
        return *this;
    }

    template<
        typename GETTER_T,
        std::enable_if_t<std::is_member_function_pointer<GETTER_T>::value>* = nullptr
    >
    type_register<T>& prop_read_only(const char* name, GETTER_T getter) {
        static_assert(std::is_base_of_v<MetaObject, T>, "T must inherit MetaObject to register properties");
        static_assert(ARGUMENT_CHECKER<GETTER_T>::arg_count == 0, "A property getter must have 0 arguments");
        
        using ReturnType = invoke_result_t<decltype(getter), T*>;
        using ReturnType_Unqualified = unqualified_type<ReturnType>;

        type_property_desc prop_desc;
        prop_desc.writable = false;
        prop_desc.readable = true;
        prop_desc.name = name;
        prop_desc.t = type_get<unqualified_type<ReturnType>>();
        prop_desc.fn_get_varying = [getter](const MetaObject* object)->varying {
            return varying::make((((T*)object)->*getter)());
        };
        prop_desc.fn_get_ptr = [getter](const MetaObject* object)->void* {
            // TODO: Try to avoid copying when possible
            // TODO: Actually wtf is this, we're returning a pointer to a temporary
            // Change it so the caller supplies a buffer of appropriate size
            const auto copy = (((T*)object)->*getter)();
            const void* p = &copy;
            return const_cast<void*>(p);
        };
        prop_desc.fn_get_value = [getter](MetaObject* object, void* out) {
            *((ReturnType_Unqualified*)out) = (((T*)object)->*getter)();
        };

        prop_desc.fn_serialize_json = [getter](void* object, nlohmann::json& j) {
            const auto&& temporary = (((T*)object)->*getter)();
            type_get<unqualified_type<ReturnType>>().serialize_json(j, (void*)&temporary);
        };
        prop_desc.fn_deserialize_json = nullptr; // Can't deserialize without a setter
        properties.push_back(prop_desc);
        return *this;
    }

    template<
        typename GETTER_T,
        typename SETTER_T, std::enable_if_t<std::is_member_function_pointer<GETTER_T>::value>* = nullptr,
        std::enable_if_t<std::is_member_function_pointer<SETTER_T>::value>* = nullptr
    >
    type_register<T>& prop(const char* name, GETTER_T getter, SETTER_T setter) {
        static_assert(std::is_base_of_v<MetaObject, T>, "T must inherit MetaObject to register properties");
        static_assert(ARGUMENT_CHECKER<GETTER_T>::arg_count == 0, "A property getter must have 0 arguments");
        static_assert(ARGUMENT_CHECKER<SETTER_T>::arg_count == 1, "A property setter must have 1 argument");
        
        using ReturnType = invoke_result_t<decltype(getter), T*>;
        using ReturnType_Unqualified = unqualified_type<ReturnType>;
        using ArgType = ARGUMENT_CHECKER<SETTER_T>::ARG_TYPE;
        static_assert(std::is_same<unqualified_type<ReturnType>, unqualified_type<ArgType>>::value, "property setter and getter return and argument types must be the same");

        type_property_desc prop_desc;
        prop_desc.writable = true;
        prop_desc.readable = true;
        prop_desc.name = name;
        prop_desc.t = type_get<unqualified_type<ReturnType>>();
        prop_desc.fn_get_varying = [getter](const MetaObject* object)->varying {
            return varying::make((((T*)object)->*getter)());
        };
        prop_desc.fn_get_ptr = [getter](const MetaObject* object)->void* {
            // TODO: Try to avoid copying when possible
            // TODO: Actually wtf is this, we're returning a pointer to a temporary
            // Change it so the caller supplies a buffer of appropriate size
            const auto copy = (((T*)object)->*getter)();
            const void* p = &copy;
            return const_cast<void*>(p);
        };
        prop_desc.fn_get_value = [getter](MetaObject* object, void* out) {
            *((ReturnType_Unqualified*)out) = (((T*)object)->*getter)();
        };

        // TODO:
        prop_desc.fn_set = [setter](MetaObject* object, const void* value) {
            using NoRefArgType = unqualified_type<ArgType>;
            (((T*)object)->*setter)(*(NoRefArgType*)value);
        };

        prop_desc.fn_serialize_json = [getter](const void* object, nlohmann::json& j) {
            const auto temporary = (((T*)object)->*getter)();
            type_get<unqualified_type<ReturnType>>().serialize_json(j, (void*)&temporary);
        };
        prop_desc.fn_deserialize_json = [setter](void* object, const nlohmann::json& j) {
            type member_type = type_get<unqualified_type<ArgType>>();
            std::vector<unsigned char> buf(member_type.get_size());
            member_type.construct(buf.data());
            member_type.deserialize_json(j, buf.data());
            (((T*)object)->*setter)(*(unqualified_type<ArgType>*)buf.data());
            member_type.destruct(buf.data());
        };
        properties.push_back(prop_desc);
        return *this;
    }

    type_register<T>& custom_serialize_json(void(*pfn_custom_serialize_json)(nlohmann::json&, const void*)) {
        this->pfn_custom_serialize_json = pfn_custom_serialize_json;
        return *this;
    }
    type_register<T>& custom_deserialize_json(void(*pfn_custom_deserialize_json)(const nlohmann::json&, void*)) {
        this->pfn_custom_deserialize_json = pfn_custom_deserialize_json;
        return *this;
    }
};


}

