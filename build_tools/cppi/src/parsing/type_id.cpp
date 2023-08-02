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
            return user_defined_type->name;
        }
    } else {
        auto t = type_specifier_flags_to_built_in_type(type_flags);
        return built_in_type_to_string(t);
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

