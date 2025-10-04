#pragma once

#include <type_traits>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include "token.hpp"


enum ATTRIB_VALUE_TYPE {
    ATTRIB_VALUE_NULL,
    ATTRIB_VALUE_NUMBER,
    ATTRIB_VALUE_VEC2,
    ATTRIB_VALUE_VEC3,
    ATTRIB_VALUE_VEC4,
    ATTRIB_VALUE_STRING,
    ATTRIB_VALUE_OBJECT,
    ATTRIB_VALUE_ARRAY
};

class iattrib_value {
    virtual bool is_type(const std::type_info& t) const { return false; }
    virtual void* get_value_ptr() const { return 0; }
public:
    virtual ~iattrib_value() {}

    virtual iattrib_value* make_copy() const { return 0; }

    virtual bool is_null() const { return false; }

    template<typename T>
    bool is() const { return is_type(typeid(T)); }
    template<typename T>
    T get() const {
        if (!is<T>()) {
            return T();
        }
        return *(T*)get_value_ptr();
    }
};

template<typename T>
class attrib_value_t : public iattrib_value {
    T value;
    bool is_type(const std::type_info& t) const override { return typeid(T) == t; }
    void* get_value_ptr() const override { return (void*)&value; }
    iattrib_value* make_copy() const override { return new attrib_value_t(*this); }
public:
    attrib_value_t(const T& val) : value(val) {}
};

class attrib_value_null : public iattrib_value {
public:
    bool is_null() const override { return true; }
    iattrib_value* make_copy() const override { return new attrib_value_null; }
};

class attrib_value {
    std::unique_ptr<iattrib_value> value;
public:
    attrib_value() : value(new attrib_value_null) {}
    attrib_value& operator=(const attrib_value& other) {
        value.reset(other.value->make_copy());
        return *this;
    }

    bool is_string() const { return value->is<std::string>(); }
    bool is_number() const { return value->is<int>() || value->is<float>(); }

    void set_string(const std::string& val) {
        value.reset(new attrib_value_t<std::string>(val));
    }
    void set_int(int val) {
        value.reset(new attrib_value_t<int>(val));
    }
    void set_float(int val) {
        value.reset(new attrib_value_t<float>(val));
    }

    std::string get_string() const { return value->get<std::string>(); }
    int get_int() const { return value->get<int>(); }
    float get_float() const { return value->get<float>(); }
};

class attribute {
    std::unordered_map<std::string, std::unique_ptr<attribute>> attribs;
    std::vector<token> tokens;
public:
    std::string name;
    std::string qualified_name;
    attrib_value value;

    attribute() {}
    attribute(const attribute& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
            );
        }
        tokens = other.tokens;
        value = other.value;
    }
    attribute(attribute&& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::move(it.second))
            );
        }
        tokens = std::move(other.tokens);
    }

    void merge(const attribute& source) {
        for (auto& it : source.attribs) {
            auto& it2 = attribs.find(it.first);
            if (it2 == attribs.end()) {
                attribs.insert(
                    std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
                );
                continue;
            }
            it2->second->merge(*it.second.get());
        }
        tokens.insert(tokens.end(), source.tokens.begin(), source.tokens.end());
    }

    int token_count() const {
        return tokens.size();
    }
    int attrib_count() const {
        return attribs.size();
    }
    attribute* getadd_attrib(const std::string& name) {
        attribute* ptr = find_attrib(name);
        if (ptr) {
            return ptr;
        }
        ptr = new attribute();
        ptr->name = name;
        ptr->qualified_name = qualified_name + "::" + name;
        attribs.insert(std::make_pair(name, std::unique_ptr<attribute>(ptr)));
        return ptr;
    }
    void add_token(const token& tok) {
        tokens.push_back(tok);
    }
    void add_tokens(const token* in_tokens, int count) {
        tokens.insert(tokens.end(), in_tokens, in_tokens + count);
    }

    attribute* find_attrib(const std::string& name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }
    const token* get_tokens() {
        return &tokens[0];
    }
    const token& get_token(int i) const {
        return tokens[i];
    }
};
class attribute_specifier {
    std::unordered_map<std::string, std::unique_ptr<attribute>> attribs;
public:
    attribute_specifier() {}
    attribute_specifier(const attribute_specifier& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
            );
        }
    }
    attribute_specifier(attribute_specifier&& other) {
        attribs.clear();
        for (auto& it : other.attribs) {
            attribs.insert(
                std::make_pair(it.first, std::move(it.second))
            );
        }
    }

    void merge(const attribute_specifier& source) {
        for (auto& it : source.attribs) {
            auto& it2 = attribs.find(it.first);
            if (it2 == attribs.end()) {
                attribs.insert(
                    std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
                );
                continue;
            }
            it2->second->merge(*it.second.get());
        }
    }

    bool empty() const {
        return attrib_count() == 0;
    }
    int attrib_count() const {
        return attribs.size();
    }
    attribute* getadd_attrib(const std::string& name) {
        attribute* ptr = find_attrib(name);
        if (ptr) {
            return ptr;
        }
        ptr = new attribute();
        ptr->name = name;
        ptr->qualified_name = name;
        attribs.insert(std::make_pair(name, std::unique_ptr<attribute>(ptr)));
        return ptr;
    }

    attribute* find_attrib(const std::string& name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }
};