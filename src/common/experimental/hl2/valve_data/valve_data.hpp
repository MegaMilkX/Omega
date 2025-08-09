#pragma once

#include <string>
#include <map>
#include <memory>
#include <variant>


class valve_data;

class valve_array {
    std::vector<std::unique_ptr<valve_data>> array_;
public:
    class iterator {
        std::vector<std::unique_ptr<valve_data>>::const_iterator it;
    public:
        iterator(std::nullptr_t nptr)
            : it() {}
        iterator(const std::vector<std::unique_ptr<valve_data>>::const_iterator& it)
            : it(it) {}

        valve_data&         operator*() { return *it->get(); }
        const valve_data&   operator*() const { return *it->get(); }
        valve_data*         operator->() { return it->get(); }
        const valve_data*   operator->() const { return it->get(); }
        iterator&           operator++() { it++; return *this; }
        bool                operator==(const iterator& other) const { return it == other.it; }
        bool                operator!=(const iterator& other) const { return it != other.it; }
    };

    valve_array() {}
    valve_array(const valve_array& other);
    valve_array& operator=(const valve_array& other);

    size_t size() const { return array_.size(); }
    bool empty() const { return array_.empty(); }
    iterator begin() const { return iterator(array_.begin()); }
    iterator end() const { return iterator(array_.end()); }

    void push_back(const valve_data& elem);

    valve_data& operator[](int i);
    const valve_data& operator[](int i) const;
};

class valve_object {
    std::map<std::string, std::unique_ptr<valve_data>> dict;
public:
    class iterator {
        std::map<std::string, std::unique_ptr<valve_data>>::const_iterator it;
    public:
        iterator(std::nullptr_t nptr)
            : it() {}
        iterator(const std::map<std::string, std::unique_ptr<valve_data>>::const_iterator& it)
            : it(it) {}

        valve_data&         operator*() { return *it->second.get(); }
        const valve_data&   operator*() const { return *it->second.get(); }
        valve_data*         operator->() { return it->second.get(); }
        const valve_data*   operator->() const { return it->second.get(); }
        iterator&           operator++() { it++; return *this; }
        bool                operator==(const iterator& other) const { return it == other.it; }
        bool                operator!=(const iterator& other) const { return it != other.it; }

        const std::string&  key() const { return it->first; }
        valve_data&         value() { return *it->second.get(); }
        const valve_data&   value() const { return *it->second.get(); }
    };

    valve_object() {}
    valve_object(const valve_object& other);
    valve_object& operator=(const valve_object& other);

    bool empty() const {
        return dict.empty();
    }

    iterator find(const std::string& key) const {
        return iterator(dict.find(key));
    }

    iterator begin() const {
        return iterator(dict.begin());
    }
    iterator end() const {
        return iterator(dict.end());
    }

    valve_data& operator[](const std::string& key);
};

struct valve_number {
    union {
        gfxm::vec3  vec3_;
        int         int_;
        float       float_;
    };

    valve_number(int val)
        : int_(val) {}
    valve_number(float val)
        : float_(val) {}
    valve_number(const gfxm::vec3& val)
        : vec3_(val) {}
};


enum valve_type {
    valve_type_null,
    valve_type_object,
    valve_type_array,
    valve_type_string,
    valve_type_int,
    valve_type_float,
    valve_type_vec3,
};


class valve_value {
    valve_type type;
    std::variant<valve_object, valve_array, std::string, valve_number> var;
public:
    valve_value(const valve_value& other)
        : type(other.type), var(other.var) {}
    valve_value(std::nullptr_t nptr)
        : type(valve_type_null) {}
    valve_value(const valve_object& obj)
        : type(valve_type_object), var(obj) {}
    valve_value(const valve_array& arr)
        : type(valve_type_array), var(arr) {}
    valve_value(const std::string& str)
        : type(valve_type_string), var(str) {}
    valve_value(int val)
        : type(valve_type_int), var(valve_number(val)) {}
    valve_value(float val)
        : type(valve_type_float), var(valve_number(val)) {}
    valve_value(const gfxm::vec3& val)
        : type(valve_type_vec3), var(valve_number(val)) {}

    valve_value& operator=(const valve_value& other) {
        type = other.type;
        var = other.var;
        return *this;
    }

    bool is_null() const { return type == valve_type_null; }
    bool is_object() const { return type == valve_type_object; }
    bool is_array() const { return type == valve_type_array; }
    bool is_string() const { return type == valve_type_string; }
    bool is_numeric() const { return type == valve_type_int || type == valve_type_float; }
    bool is_vec3() const { return type == valve_type_vec3; }

    valve_object& as_object() {
        return std::get<valve_object>(var);
    }
    valve_array& as_array() {
        return std::get<valve_array>(var);
    }
    std::string as_string() {
        return std::get<std::string>(var);
    }
    int as_int() {
        if (type == valve_type_int) {
            return std::get<valve_number>(var).int_;
        } else if (type == valve_type_float) {
            return int(std::get<valve_number>(var).float_);
        } else {
            return 0;
        }
    }
    float as_float() {
        if (type == valve_type_int) {
            return float(std::get<valve_number>(var).int_);
        } else if (type == valve_type_float) {
            return std::get<valve_number>(var).float_;
        } else {
            return .0f;
        }
    }
    gfxm::vec3 as_vec3() {
        return std::get<valve_number>(var).vec3_;
    }
};


class valve_data {
    valve_value var;

    std::string indent_str(int indent) {
        return std::string(indent * 2, ' ');
    }

    void to_string_impl(std::ostringstream& oss, int indent) {
        if (is_null()) {
            oss << "null";
        } else if (is_string()) {
            oss << "\"" << as_string() << "\"";
        } else if (is_numeric()) {
            oss << "\"" << "numeric to_string() not implemented" << "\"";
        } else if (is_vec3()) {
            oss << "\"" << "vec3 to_string() not implemented" << "\"";
        } else if (is_object()) {
            oss << "{";
            if(!as_object().empty()) oss << "\n";
            for (auto it = begin(); it != end(); ++it) {
                oss << indent_str(indent + 1) << "\"" << it.key() << "\" ";
                valve_data& value = it.value();

                if (value.is_object()) {
                    value.to_string_impl(oss, indent + 1);
                    oss << "\n";
                } else {
                    value.to_string_impl(oss, 0);
                    oss << "\n";
                }
            }
            if(!as_object().empty()) oss << indent_str(indent);
            oss << "}";
        }
    }

public:
    valve_data()
        : var(nullptr) {}
    valve_data(std::nullptr_t nptr)
        : var(nptr) {}
    valve_data(const valve_object& obj)
        : var(obj) {}
    valve_data(const std::string& str)
        : var(str) {}
    valve_data(const valve_data& other)
        :var(other.var) {}

    valve_data& operator=(const valve_data& other) {
        var = other.var;
        return *this;
    }

    operator bool() const {
        return !var.is_null();
    }

    std::string to_string() {
        std::ostringstream oss;
        to_string_impl(oss, 0);
        return oss.str();
    }

    void push_back(const valve_data& elem) {
        if (!var.is_array()) {
            var = valve_value(valve_array());
        }
        var.as_array().push_back(elem);
    }

    bool is_null() const    { return var.is_null(); }
    bool is_object() const  { return var.is_object(); }
    bool is_array() const   { return var.is_array(); }
    bool is_string() const  { return var.is_string(); }
    bool is_numeric() const { return var.is_numeric(); }
    bool is_vec3() const    { return var.is_vec3(); }
    
    valve_object&       as_object() { return var.as_object(); }
    valve_array&        as_array()  { return var.as_array(); }
    std::string         as_string() { return var.as_string(); }
    int                 as_int()    { return var.as_int(); }
    float               as_float()  { return var.as_float(); }
    gfxm::vec3          as_vec3()   { return var.as_vec3(); }
    
    bool empty() {
        if(is_object()) return as_object().empty();
        if(is_array()) return as_array().empty();
        return true;
    }

    // Array access
    size_t size() {
        if (!is_array()) {
            return 0;
        }
        return as_array().size();
    }
    valve_data& operator[](int i) {
        static valve_data dummy = nullptr;
        if (!is_array()) {
            return dummy;
        }
        return as_array()[i];
    }

    // Object access
    valve_data& operator[](const char* key) {
        if (!var.is_object()) {
            var = valve_value(valve_object());
        }
        return var.as_object()[key];
    }
    valve_data& operator[](const std::string& key) {
        if (!var.is_object()) {
            var = valve_value(valve_object());
        }
        return var.as_object()[key];
    }
    valve_object::iterator find(const std::string& key) {
        if (!is_object()) {
            return valve_object::iterator(nullptr);
        }
        return as_object().find(key);
    }
    valve_object::iterator begin() {
        if (!is_object()) {
            return valve_object::iterator(nullptr);
        }
        return as_object().begin();
    }
    valve_object::iterator end() {
        if (!is_object()) {
            return valve_object::iterator(nullptr);
        }
        return as_object().end();
    }

    std::string get_string(const std::string& key, const std::string& default_value = "") {
        if (!var.is_object()) {
            return default_value;
        }
        auto it = var.as_object().find(key);
        if (it == var.as_object().end()) {
            return default_value;
        }
        if (!(*it).is_string()) {
            return default_value;
        }
        return (*it).as_string();
    }
    int         get_int(const std::string& key, int default_value = 0) {
        if (!var.is_object()) {
            return default_value;
        }
        auto it = var.as_object().find(key);
        if (it == var.as_object().end()) {
            return default_value;
        }
        if (!(*it).is_numeric()) {
            return default_value;
        }
        return (*it).as_int();
    }
    float       get_float(const std::string& key, float default_value = .0f) {
        if (!var.is_object()) {
            return default_value;
        }
        auto it = var.as_object().find(key);
        if (it == var.as_object().end()) {
            return default_value;
        }
        if (!(*it).is_numeric()) {
            return default_value;
        }
        return (*it).as_float();
    }
    gfxm::vec3  get_vec3(const std::string& key, const gfxm::vec3& default_value = gfxm::vec3(0, 0, 0)) {
        if (!var.is_object()) {
            return default_value;
        }
        auto it = var.as_object().find(key);
        if (it == var.as_object().end()) {
            return default_value;
        }
        if (!(*it).is_vec3()) {
            return default_value;
        }
        return (*it).as_vec3();
    }
};

inline valve_array::valve_array(const valve_array& other) {
    array_.clear();
    array_.resize(other.array_.size());
    for (int i = 0; i < array_.size(); ++i) {
        array_[i].reset(new valve_data(*other.array_[i].get()));
    }
}
inline valve_array& valve_array::operator=(const valve_array& other) {
    array_.clear();
    array_.resize(other.array_.size());
    for (int i = 0; i < array_.size(); ++i) {
        array_[i].reset(new valve_data(*other.array_[i].get()));
    }
    return *this;
}
inline void valve_array::push_back(const valve_data& elem) {
    array_.push_back(std::unique_ptr<valve_data>(new valve_data(elem)));
}
inline valve_data& valve_array::operator[](int i) {
    return *array_[i].get();
}
inline const valve_data& valve_array::operator[](int i) const {
    return *array_[i].get();
}

inline valve_object::valve_object(const valve_object& other) {
    dict.clear();
    for (auto& kv : other.dict) {
        dict.insert(std::make_pair(kv.first, std::unique_ptr<valve_data>(new valve_data(*kv.second.get()))));
    }
}
inline valve_object& valve_object::operator=(const valve_object& other) {
    dict.clear();
    for (auto& kv : other.dict) {
        dict.insert(std::make_pair(kv.first, std::unique_ptr<valve_data>(new valve_data(*kv.second.get()))));
    }
    return *this;
}

inline valve_data& valve_object::operator[](const std::string& key) {
    auto it = dict.find(key);
    if (it == dict.end()) {
        it = dict.insert(std::make_pair(key, std::unique_ptr<valve_data>(new valve_data(nullptr)))).first;
    }
    return *it->second.get();
}


