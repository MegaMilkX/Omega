#include "type_id.hpp"
#include "symbol_table.hpp"

#include "dbg.hpp"


std::string decl_type::make_string() const {
    if (type_flags & fl_user_defined) {
        if (!user_defined_type) {
            return "<error-type>";
        } else if(user_defined_type->is<symbol_placeholder>()) {
            symbol_placeholder* plhd = (symbol_placeholder*)user_defined_type.get();
            std::string str;
            for (int i = 0; i < plhd->token_cache.size(); ++i) {
                str += plhd->token_cache[i].str;
            }
            return str;
        } else {
            std::string ret;
            ret += user_defined_type->global_qualified_name;
            return ret;
        }
    } else {
        auto t = type_specifier_flags_to_built_in_type(type_flags);
        std::string ret;
        ret += built_in_type_to_string(t);
        return ret;
    }
}

void decl_type::dbg_print() const {
    if (type_flags & fl_user_defined) {
        if (!user_defined_type) {
            dbg_printf_color("<error-type>", DBG_RED);
        } else if(user_defined_type->is<symbol_placeholder>()) {
            symbol_placeholder* plhd = (symbol_placeholder*)user_defined_type.get();
            for (int i = 0; i < plhd->token_cache.size(); ++i) {
                dbg_printf_color("%s", DBG_RED | DBG_GREEN | DBG_INTENSITY, plhd->token_cache[i].str.c_str());
            }
        } else {
            dbg_printf_color("%s", 0x17, user_defined_type->name.c_str());            
        }
    } else {
        auto t = type_specifier_flags_to_built_in_type(type_flags);
        dbg_printf_color("%s", 0x17, built_in_type_to_string(t));
    }
}


std::string type_id::make_string(bool strip_ref, bool strip_ptr, bool strip_cv) const {
    std::string str;
    for (int i = 0; i < parts.size(); ++i) {
        type_id_part* pt = parts[i].get();
        bool not_last = i < parts.size() - 1;

            if (pt->cast_to<type_id_part_array>()) {
            str = str + "[]";
        } else if (pt->cast_to<type_id_part_function>()) {
            str = str + pt->make_string(strip_cv);
        } else if (pt->cast_to<type_id_part_object>()) {
            str = pt->make_string(strip_cv) + str;
        } else if (pt->cast_to<type_id_part_ptr>() && !strip_ptr) {
            if(not_last && parts[i + 1]->cast_to<type_id_part_function>()) {
                str = "(*" + str + ")";
            } else {
                str = "*" + str;
            }
        } else if(pt->cast_to<type_id_part_member_ptr>() && !strip_ptr) {
            auto member_ptr = pt->cast_to<type_id_part_member_ptr>();
            if (not_last && parts[i + 1]->cast_to<type_id_part_function>()) {
                str = "(" + member_ptr->owner->global_qualified_name + "::*" + str + ")";
            } else {
                str = member_ptr->owner->global_qualified_name + "::*" + str;
            }
        } else if (pt->cast_to<type_id_part_ref>() && !strip_ref) {
            if (not_last && parts[i + 1]->cast_to<type_id_part_function>()) {
                str = "(&" + str + ")";
            } else {
                str = "&" + str;
            }
        }
    }

    return str;
}