#include "decl_specifier.hpp"
#include "type_specifier.hpp"
#include "attribute_specifier.hpp"
#include "class_specifier.hpp"
#include "enum_specifier.hpp"
#include "parse_exception.hpp"


bool eat_storage_class_specifier(parse_state& ps, decl_spec& decl_spec_, decl_type& object_type_) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type == kw_register) {
        if (!decl_spec_.is_decl_compatible(DECL_STORAGE_REGISTER)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_STORAGE_REGISTER, COMPAT_STORAGE_REGISTER);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_static) {
        if (!decl_spec_.is_decl_compatible(DECL_STORAGE_STATIC)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_STORAGE_STATIC, COMPAT_STORAGE_STATIC);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_thread_local) {
        if (!decl_spec_.is_decl_compatible(DECL_STORAGE_THREAD_LOCAL)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_STORAGE_THREAD_LOCAL, COMPAT_STORAGE_THREAD_LOCAL);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_extern) {
        if (!decl_spec_.is_decl_compatible(DECL_STORAGE_EXTERN)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_STORAGE_EXTERN, COMPAT_STORAGE_EXTERN);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_mutable) {
        if (!decl_spec_.is_decl_compatible(DECL_STORAGE_MUTABLE)) {
            throw parse_exception("Incompatible storage class specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_STORAGE_MUTABLE, COMPAT_STORAGE_MUTABLE);
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_function_specifier(parse_state& ps, decl_spec& decl_spec_) {
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type != tt_keyword) {
        ps.rewind_();
        return false;
    }
    if (tok.kw_type == kw_inline) {
        if (!decl_spec_.is_decl_compatible(DECL_FN_INLINE)) {
            throw parse_exception("Incompatible function specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_FN_INLINE, COMPAT_FN_INLINE);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_virtual) {
        if (!decl_spec_.is_decl_compatible(DECL_FN_VIRTUAL)) {
            throw parse_exception("Incompatible function specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_FN_VIRTUAL, COMPAT_FN_VIRTUAL);
        ps.pop_rewind_point();
        return true;
    }
    if (tok.kw_type == kw_explicit) {
        if (!decl_spec_.is_decl_compatible(DECL_FN_EXPLICIT)) {
            throw parse_exception("Incompatible function specifier", tok);
        }
        decl_spec_.set_decl_flag(DECL_FN_EXPLICIT, COMPAT_FN_EXPLICIT);
        ps.pop_rewind_point();
        return true;
    }
    ps.rewind_();
    return false;
}
bool eat_decl_specifier(parse_state& ps, decl_spec& decl_spec_, decl_type& object_type_) {
    if (eat_storage_class_specifier(ps, decl_spec_, object_type_)) {
        return true;
    }
    if (eat_type_specifier(ps, decl_spec_, object_type_)) {
        return true;
    }
    if (eat_function_specifier(ps, decl_spec_)) {
        return true;
    }
    ps.push_rewind_point();
    token tok = ps.next_token();
    if (tok.type == tt_keyword) {
        if (tok.kw_type == kw_friend) {
            if (!decl_spec_.is_decl_compatible(DECL_FRIEND)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            decl_spec_.set_decl_flag(DECL_FRIEND, COMPAT_FRIEND);
            ps.pop_rewind_point();
            return true;
        }
        if (tok.kw_type == kw_typedef) {
            if (!decl_spec_.is_decl_compatible(DECL_TYPEDEF)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            decl_spec_.set_decl_flag(DECL_TYPEDEF, COMPAT_TYPEDEF);
            ps.pop_rewind_point();
            return true;
        }
        if (tok.kw_type == kw_constexpr) {
            if (!decl_spec_.is_decl_compatible(DECL_CONSTEXPR)) {
                throw parse_exception("Incompatible specifier", tok);
            }
            decl_spec_.set_decl_flag(DECL_CONSTEXPR, COMPAT_CONSTEXPR);
            ps.pop_rewind_point();
            return true;
        }
    }
    ps.rewind_();
    return false;
}
bool eat_decl_specifier_seq(parse_state& ps, decl_spec& ds, decl_type& dt) {
    if (!eat_decl_specifier(ps, ds, dt)) {
        return false;
    }
    while (eat_decl_specifier(ps, ds, dt)) {

    }
    eat_attribute_specifier_seq(ps);
    return true;
}