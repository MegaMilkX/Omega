#pragma once

#include <assert.h>
#include <memory>
#include <string>
#include <vector>
#include <stdio.h>
#include "dbg.hpp"
#include "preprocessor/pp_file.hpp"

const int fl_char        = 0x0001;//0b0000000000000001; // 0x0001
const int fl_char16_t    = 0x0002;//0b0000000000000010; // 0x0002
const int fl_char32_t    = 0x0004;//0b0000000000000100; // 0x0004
const int fl_wchar_t     = 0x0008;//0b0000000000001000; // 0x0008
const int fl_bool        = 0x0010;//0b0000000000010000; // 0x0010
const int fl_short       = 0x0020;//0b0000000000100000; // 0x0020
const int fl_int         = 0x0040;//0b0000000001000000; // 0x0040
const int fl_long        = 0x0080;//0b0000000010000000; // 0x0080
const int fl_longlong    = 0x0100;//0b0000000100000000; // 0x0100
const int fl_signed      = 0x0200;//0b0000001000000000; // 0x0200
const int fl_unsigned    = 0x0400;//0b0000010000000000; // 0x0400
const int fl_float       = 0x0800;//0b0000100000000000; // 0x0800
const int fl_double      = 0x1000;//0b0001000000000000; // 0x1000
const int fl_void        = 0x2000;//0b0010000000000000; // 0x2000
const int fl_auto        = 0x4000;//0b0100000000000000; // 0x4000
const int fl_user_defined      = 0x8000;//0b1000000000000000; // 0x8000

const int compat_char     = (fl_unsigned | fl_signed);
const int compat_char16_t = 0;
const int compat_char32_t = 0;
const int compat_wchar_t  = 0;
const int compat_bool     = 0;
const int compat_short    = (fl_unsigned | fl_signed | fl_int);
const int compat_int      = (fl_unsigned | fl_signed | fl_short | fl_long | fl_longlong);
const int compat_long     = (fl_unsigned | fl_signed | fl_int | fl_double | fl_long);
const int compat_longlong = (fl_unsigned | fl_signed | fl_int);
const int compat_signed   = (fl_char | fl_short | fl_int | fl_long | fl_longlong);
const int compat_unsigned = (fl_char | fl_short | fl_int | fl_long | fl_longlong);
const int compat_float    = 0;
const int compat_double   = (fl_long);
const int compat_void     = 0;
const int compat_auto     = 0;
const int compat_custom   = 0;

const int DECL_CV_CONST             = 0x0001;
const int DECL_CV_VOLATILE          = 0x0002;
const int DECL_FRIEND               = 0x0004;
const int DECL_TYPEDEF              = 0x0008;
const int DECL_CONSTEXPR            = 0x0010;
const int DECL_FN_INLINE            = 0x0020;
const int DECL_FN_VIRTUAL           = 0x0040;
const int DECL_FN_EXPLICIT          = 0x0080;
const int DECL_STORAGE_REGISTER     = 0x0100;
const int DECL_STORAGE_STATIC       = 0x0200;
const int DECL_STORAGE_THREAD_LOCAL = 0x0400;
const int DECL_STORAGE_EXTERN       = 0x0800;
const int DECL_STORAGE_MUTABLE      = 0x1000;

const int COMPAT_CV_CONST               = ~0 & ~(DECL_STORAGE_MUTABLE);
const int COMPAT_CV_VOLATILE            = ~0; // Compatible with every other specifier and itself
const int COMPAT_FRIEND                 = (DECL_CV_CONST | DECL_CV_VOLATILE | DECL_CONSTEXPR);
const int COMPAT_TYPEDEF                = (DECL_CV_CONST | DECL_CV_VOLATILE);
const int COMPAT_CONSTEXPR              = ~(DECL_TYPEDEF | DECL_FN_VIRTUAL);
const int COMPAT_FN_INLINE              = (DECL_CV_CONST | DECL_CV_VOLATILE);
const int COMPAT_FN_VIRTUAL             = (DECL_CV_CONST | DECL_CV_VOLATILE);
const int COMPAT_FN_EXPLICIT            = (DECL_CV_CONST | DECL_CV_VOLATILE);
const int COMPAT_STORAGE_REGISTER       = (DECL_CV_CONST | DECL_CV_VOLATILE);
const int COMPAT_STORAGE_STATIC         = (DECL_CV_CONST | DECL_CV_VOLATILE | DECL_STORAGE_THREAD_LOCAL);
const int COMPAT_STORAGE_THREAD_LOCAL   = (DECL_CV_CONST | DECL_CV_VOLATILE | DECL_STORAGE_EXTERN | DECL_STORAGE_STATIC);
const int COMPAT_STORAGE_EXTERN         = (DECL_CV_CONST | DECL_CV_VOLATILE | DECL_STORAGE_THREAD_LOCAL);
const int COMPAT_STORAGE_MUTABLE        = (DECL_CV_VOLATILE);

enum BUILT_IN_TYPE {
    BUILT_IN_TYPE_NONE,
    BUILT_IN_TYPE_USER_DEFINED,
    BUILT_IN_TYPE_CHAR,
    BUILT_IN_TYPE_UCHAR,
    BUILT_IN_TYPE_SCHAR,
    BUILT_IN_TYPE_CHAR16_T,
    BUILT_IN_TYPE_CHAR32_T,
    BUILT_IN_TYPE_BOOL,
    BUILT_IN_TYPE_UINT,
    BUILT_IN_TYPE_INT,
    BUILT_IN_TYPE_USHORT,
    BUILT_IN_TYPE_ULONG,
    BUILT_IN_TYPE_ULONGLONG,
    BUILT_IN_TYPE_LONG,
    BUILT_IN_TYPE_LONGLONG,
    BUILT_IN_TYPE_SHORT,
    BUILT_IN_TYPE_WCHAR_T,
    BUILT_IN_TYPE_FLOAT,
    BUILT_IN_TYPE_DOUBLE,
    BUILT_IN_TYPE_LONG_DOUBLE,
    BUILT_IN_TYPE_VOID,
    BUILT_IN_TYPE_AUTO
};
inline BUILT_IN_TYPE type_specifier_flags_to_built_in_type(int type_flags) {
    auto& f = type_flags;
    switch (f) {
    case fl_char:
        return BUILT_IN_TYPE_CHAR;
    case fl_char | fl_unsigned:
        return BUILT_IN_TYPE_UCHAR;
    case fl_char | fl_signed:
        return BUILT_IN_TYPE_SCHAR;
    case fl_char16_t:
        return BUILT_IN_TYPE_CHAR16_T;
    case fl_char32_t:
        return BUILT_IN_TYPE_CHAR32_T;
    case fl_bool:
        return BUILT_IN_TYPE_BOOL;
    case fl_unsigned:
        return BUILT_IN_TYPE_UINT;
    case fl_unsigned | fl_int:
        return BUILT_IN_TYPE_UINT;
    case fl_signed:
        return BUILT_IN_TYPE_INT;
    case fl_signed | fl_int:
        return BUILT_IN_TYPE_INT;
    case fl_int:
        return BUILT_IN_TYPE_INT;
    case fl_unsigned | fl_short | fl_int:
        return BUILT_IN_TYPE_USHORT;
    case fl_unsigned | fl_short:
        return BUILT_IN_TYPE_USHORT;
    case fl_unsigned | fl_long | fl_int:
        return BUILT_IN_TYPE_ULONG;
    case fl_unsigned | fl_long:
        return BUILT_IN_TYPE_ULONG;
    case fl_unsigned | fl_longlong | fl_int:
        return BUILT_IN_TYPE_ULONGLONG;
    case fl_unsigned | fl_longlong:
        return BUILT_IN_TYPE_ULONGLONG;
    case fl_signed | fl_long | fl_int:
        return BUILT_IN_TYPE_LONG;
    case fl_signed | fl_long:
        return BUILT_IN_TYPE_LONG;
    case fl_signed | fl_longlong | fl_int:
        return BUILT_IN_TYPE_LONGLONG;
    case fl_signed | fl_longlong:
        return BUILT_IN_TYPE_LONGLONG;
    case fl_longlong | fl_int:
        return BUILT_IN_TYPE_LONGLONG;
    case fl_longlong:
        return BUILT_IN_TYPE_LONGLONG;
    case fl_long | fl_int:
        return BUILT_IN_TYPE_LONG;
    case fl_long:
        return BUILT_IN_TYPE_LONG;
    case fl_signed | fl_short | fl_int:
        return BUILT_IN_TYPE_SHORT;
    case fl_signed | fl_short:
        return BUILT_IN_TYPE_SHORT;
    case fl_short | fl_int:
        return BUILT_IN_TYPE_SHORT;
    case fl_short:
        return BUILT_IN_TYPE_SHORT;
    case fl_wchar_t:
        return BUILT_IN_TYPE_WCHAR_T;
    case fl_float:
        return BUILT_IN_TYPE_FLOAT;
    case fl_double:
        return BUILT_IN_TYPE_DOUBLE;
    case fl_long | fl_double:
        return BUILT_IN_TYPE_LONG_DOUBLE;
    case fl_void:
        return BUILT_IN_TYPE_VOID;
    case fl_auto:
        return BUILT_IN_TYPE_AUTO;
    case fl_user_defined:
        return BUILT_IN_TYPE_USER_DEFINED;
    default:
        return BUILT_IN_TYPE_NONE;
    }
}
inline const char* built_in_type_to_string(BUILT_IN_TYPE type) {
    switch (type) {
    case BUILT_IN_TYPE_NONE: return "<no-type>";
    case BUILT_IN_TYPE_USER_DEFINED: return "<user-defined>";
    case BUILT_IN_TYPE_CHAR: return "char";
    case BUILT_IN_TYPE_UCHAR: return "unsigned char";
    case BUILT_IN_TYPE_SCHAR: return "signed char";
    case BUILT_IN_TYPE_CHAR16_T: return "char16_t";
    case BUILT_IN_TYPE_CHAR32_T: return "char32_t";
    case BUILT_IN_TYPE_BOOL: return "bool";
    case BUILT_IN_TYPE_UINT: return "unsigned int";
    case BUILT_IN_TYPE_INT: return "int";
    case BUILT_IN_TYPE_USHORT: return "unsigned short int";
    case BUILT_IN_TYPE_ULONG: return "unsigned long int";
    case BUILT_IN_TYPE_ULONGLONG: return "unsigned long long int";
    case BUILT_IN_TYPE_LONG: return "long int";
    case BUILT_IN_TYPE_LONGLONG: return "long long int";
    case BUILT_IN_TYPE_SHORT: return "short int";
    case BUILT_IN_TYPE_WCHAR_T: return "wchar_t";
    case BUILT_IN_TYPE_FLOAT: return "float";
    case BUILT_IN_TYPE_DOUBLE: return "double";
    case BUILT_IN_TYPE_LONG_DOUBLE: return "long double";
    case BUILT_IN_TYPE_VOID: return "void";
    case BUILT_IN_TYPE_AUTO: return "auto";
    };
    assert(false);
    return "";
}


struct decl_spec {
    int decl_flags = 0;
    int decl_compat_flags = ~0;

    void set_decl_flag(int flag, int compat_flags) {
        decl_flags |= flag;
        decl_compat_flags &= compat_flags;
    }
    bool has_decl_flag(int flag) {
        return decl_flags & flag;
    }
    bool is_decl_compatible(int flag) {
        return (decl_compat_flags & flag) == flag;
    }
};

class symbol;
struct decl_type {
    int type_flags = 0;
    int compatibility_flags = ~0;
    // Only used if type_flags & fl_user_defined
    std::shared_ptr<symbol> user_defined_type;
    // Only true when this type is built from a class definition
    // Used to determine if we should try to parse a declarator in a template decl
    bool is_type_definition = false;
    // TODO:
    // bool is_placeholder = false;
    // std::vector<token> placeholder_tokens

    std::string make_string() const;
    void dbg_print() const;
};

#define GET_TYPE \
const std::type_info& get_type() const override { return typeid(decltype(*this)); }

struct type_id_part {
    virtual const std::type_info& get_type() const { return typeid(decltype(*this)); }
    template<typename NODE_T>
    NODE_T* cast_to() const { if (get_type() == typeid(NODE_T)) return (NODE_T*)this; else return 0; }
    
    type_id_part() {}
    virtual ~type_id_part() {}

    bool equals(const type_id_part* other) const {
        if (get_type() != other->get_type()) {
            return false;
        }
        return equals_impl(other);
    }
    virtual bool equals_impl(const type_id_part* other) const { return true; }

    virtual type_id_part* make_copy() const = 0;

    virtual std::string make_string(bool strip_cv = false) const { return "<not implemented>"; }

    virtual void dbg_print() const = 0;
};

struct type_id_part_array;
struct type_id_part_function;
struct type_id_part_object;
struct type_id_part_ptr;
struct type_id_part_member_ptr;
struct type_id_part_ref;
// If the first part is a function, then type_id represents a function type
// The last part should always be an object part
class type_id {
    std::vector<std::unique_ptr<type_id_part>> parts;
public:
    type_id() {}
    type_id(const type_id& other) {
        parts.clear();
        parts.resize(other.parts.size());
        for (int i = 0; i < other.parts.size(); ++i) {
            parts[i].reset(other.parts[i]->make_copy());
        }
    }
    type_id& operator=(const type_id& other) {
        parts.clear();
        parts.resize(other.parts.size());
        for (int i = 0; i < other.parts.size(); ++i) {
            parts[i].reset(other.parts[i]->make_copy());
        }
        return *this;
    }
    bool operator ==(const type_id& other) const {
        if (parts.size() != other.parts.size()) {
            return false;
        }
        for (int i = 0; i < parts.size(); ++i) {
            auto pa = parts[i].get();
            auto pb = parts[i].get();
            if (!pa->equals(pb)) {
                return false;
            }
        }
        return true;
    }
    bool operator !=(const type_id& other) const {
        return !(*this == other);
    }

    bool is_func() const {
        return !empty() && parts[0]->cast_to<type_id_part_function>() != 0;
    }

    type_id get_return_type() const {
        type_id ret;
        int i = 0;
        while (parts[i]->cast_to<type_id_part_function>() == 0) {
            ++i;
        }
        ++i;
        if (i >= parts.size()) {
            return ret;
        }
        for (int j = i; j < parts.size(); ++j) {
            ret.parts.push_back(std::unique_ptr<type_id_part>(parts[j]->make_copy()));
        }
        return ret;
    }

    bool empty() const {
        return parts.empty();
    }
    int size() const {
        return (int)parts.size();
    }
    std::unique_ptr<type_id_part>& operator[](int i) {
        return parts[i];
    }

    template<typename T>
    T* push_back() {
        auto p = new T();
        parts.push_back(std::unique_ptr<T>(p));
        return p;
    }
    void push_back(const type_id& other) {
        int original_size = parts.size();
        parts.resize(original_size + other.parts.size());
        for (int i = original_size; i < parts.size(); ++i) {
            parts[i].reset(other.parts[i - original_size]->make_copy());
        }
    }
    template<typename T>
    T* push_front() {
        auto p = new T();
        parts.insert(parts.begin(), std::unique_ptr<T>(p));
        return p;
    }
    void pop_back() {
        assert(!empty());
        parts.pop_back();
    }

    void copy(const type_id& other) {
        parts.clear();
        parts.resize(other.parts.size());
        for (int i = 0; i < other.parts.size(); ++i) {
            parts[i].reset(other.parts[i]->make_copy());
        }
    }
    void move_merge_back(type_id& other) {
        int original_size = parts.size();
        parts.resize(original_size + other.parts.size());
        for (int i = original_size; i < parts.size(); ++i) {
            parts[i] = std::move(other.parts[i - original_size]);
        }
        other.parts.clear();
    }
    type_id_part* get_first() {
        return empty() ? 0 : parts[0].get();
    }
    type_id_part* get_last() {
        return empty() ? 0 : parts[parts.size() - 1].get();
    }
    const type_id_part* get_first() const {
        return empty() ? 0 : parts[0].get();
    }
    const type_id_part* get_last() const {
        return empty() ? 0 : parts[parts.size() - 1].get();
    }
    std::string make_string(bool strip_ref = false, bool strip_ptr = false, bool strip_cv = false) const;
    void dbg_print() const {
        if (empty()) {
            dbg_printf_color("<empty-type-id>", DBG_RED);
            return;
        }
        
        //printf("%s", make_string().c_str());
        
        parts[0]->dbg_print();
        for (int i = 1; i < parts.size(); ++i) {
            printf(" ");
            parts[i]->dbg_print();
        }
    }
};

struct declarator {
    const PP_FILE* file = 0;
    type_id decl_type_;
    std::string name;
    std::shared_ptr<symbol> sym;

    bool is_func() const {
        return decl_type_.is_func();
    }
    void dbg_print_as_param() const {
        decl_type_.dbg_print();
    }
};


struct type_id_part_ptr : public type_id_part {
    GET_TYPE;
    int cv_flags = 0;

    virtual bool equals_impl(const type_id_part* other) const { 
        // TODO: Check cv flags?
        return true; 
    }

    type_id_part* make_copy() const override {
        return new type_id_part_ptr(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        return "<not implemented>"; 
    }

    void dbg_print() const override {
        dbg_printf_color("a pointer to", 8);
    }
};
struct type_id_part_member_ptr : public type_id_part {
    GET_TYPE;
    int cv_flags = 0;
    std::shared_ptr<symbol> owner;

    virtual bool equals_impl(const type_id_part* other) const {
        // TODO: Check cv flags?
        return true;
    }

    type_id_part* make_copy() const override {
        return new type_id_part_member_ptr(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        return "<not implemented>";
    }

    void dbg_print() const override {
        dbg_printf_color("a member pointer to", 8);
    }
};
struct type_id_part_ref : public type_id_part {
    GET_TYPE;
    int cv_flags = 0;

    virtual bool equals_impl(const type_id_part* other) const {
        // TODO: Check cv flags?
        return true;
    }

    type_id_part* make_copy() const override {
        return new type_id_part_ref(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        return "<not implemented>";
    }

    void dbg_print() const override {
        dbg_printf_color("a reference to", 8);
    }
};
struct type_id_part_array : public type_id_part {
    GET_TYPE;

    virtual bool equals_impl(const type_id_part* other) const {
        // TODO: Check length?
        return true;
    }
    type_id_part* make_copy() const override {
        return new type_id_part_array(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        return "<not implemented>";
    }

    void dbg_print() const override {
        dbg_printf_color("an array of", 8);
    }
};
class symbol_table;
struct type_id_part_function : public type_id_part {
    GET_TYPE;
    std::vector<declarator> parameters;
    std::shared_ptr<symbol_table> parameter_scope;
    int cv_flags = 0;

    virtual bool equals_impl(const type_id_part* other) const {
        type_id_part_function* other_ = (type_id_part_function*)other;
        if (parameters.size() != other_->parameters.size()) {
            return false;
        }
        for (int i = 0; i < parameters.size(); ++i) {
            auto& a = parameters[i].decl_type_;
            auto& b = other_->parameters[i].decl_type_;
            if (a != b) {
                return false;
            }
        }
        return true;
    }
    type_id_part* make_copy() const override {
        return new type_id_part_function(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        std::string str = "(";
        if (!parameters.empty()) {
            str += parameters[0].decl_type_.make_string();
            for (int i = 1; i < parameters.size(); ++i) {
                str += ", ";
                str += parameters[i].decl_type_.make_string();
            }
        }
        str = str + ")";
        if ((cv_flags & DECL_CV_CONST) != 0 && !strip_cv) {
            str += " const";
        }
        return str;
    }

    void dbg_print() const override {
        dbg_printf_color("a function of (", DBG_RED | DBG_BLUE | DBG_INTENSITY);
        if (!parameters.empty()) {
            parameters[0].dbg_print_as_param();
            for (int i = 1; i < parameters.size(); ++i) {
                dbg_printf_color(", ", 8);
                parameters[i].dbg_print_as_param();
            }
        } else {
            dbg_printf_color("void", 0x17);
        }
        dbg_printf_color(") returning", DBG_RED | DBG_BLUE | DBG_INTENSITY);
    }
};
struct type_id_part_object : public type_id_part {
    GET_TYPE;
    decl_type object_type_;
    decl_spec decl_specifiers;

    virtual bool equals_impl(const type_id_part* other) const {
        type_id_part_object* p = (type_id_part_object*)other;
        if (object_type_.type_flags != p->object_type_.type_flags) {
            return false;
        }
        if (object_type_.user_defined_type != p->object_type_.user_defined_type) {
            return false;
        }
        return true;
    }
    type_id_part* make_copy() const override {
        return new type_id_part_object(*this);
    }

    std::string make_string(bool strip_cv = false) const override {
        std::string ret;
        if ((decl_specifiers.decl_flags & DECL_CV_CONST) != 0 && !strip_cv) {
            ret += "const ";
        }
        ret += object_type_.make_string();
        return ret;
    }

    void dbg_print() const override {
        dbg_printf_color("an object of type ", 8);
        object_type_.dbg_print();
    }
};

