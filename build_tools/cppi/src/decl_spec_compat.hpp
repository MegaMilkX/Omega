#pragma once

#include <assert.h>


enum e_decl_type {
    e_decl_char                  = 0x00001,
    e_decl_char16_t              = 0x00002,
    e_decl_char32_t              = 0x00004,
    e_decl_wchar_t               = 0x00008,
    e_decl_bool                  = 0x00010,
    e_decl_short                 = 0x00020,
    e_decl_int                   = 0x00040,
    e_decl_long                  = 0x00080,
    e_decl_longlong              = 0x00100,
    e_decl_signed                = 0x00200,
    e_decl_unsigned              = 0x00400,
    e_decl_float                 = 0x00800,
    e_decl_double                = 0x01000,
    e_decl_void                  = 0x02000,
    e_decl_auto                  = 0x04000,
    e_decl_decltype              = 0x08000,
    e_decl_user_defined          = 0x10000,
};

enum e_decl_specifier {
    e_cv_const              = 0x0001,
    e_cv_volatile           = 0x0002,
    e_friend                = 0x0004,
    e_typedef               = 0x0008,
    e_constexpr             = 0x0010,
    e_fn_inline             = 0x0020,
    e_fn_virtual            = 0x0040,
    e_fn_explicit           = 0x0080,
    e_storage_register      = 0x0100,
    e_storage_static        = 0x0200,
    e_storage_thread_local  = 0x0400,
    e_storage_extern        = 0x0800,
    e_storage_mutable       = 0x1000,
};

inline int get_type_compatibility(e_decl_type t) {
    switch (t) {
    case e_decl_char         : return (e_decl_unsigned | e_decl_signed);
    case e_decl_char16_t     : return 0;
    case e_decl_char32_t     : return 0;
    case e_decl_wchar_t      : return 0;
    case e_decl_bool         : return 0;
    case e_decl_short        : return (e_decl_unsigned | e_decl_signed | e_decl_int);
    case e_decl_int          : return (e_decl_unsigned | e_decl_signed | e_decl_short | e_decl_long | e_decl_longlong);
    case e_decl_long         : return (e_decl_unsigned | e_decl_signed | e_decl_int | e_decl_double | e_decl_long | e_decl_longlong);
    case e_decl_longlong     : return (e_decl_unsigned | e_decl_signed | e_decl_int);
    case e_decl_signed       : return (e_decl_char | e_decl_short | e_decl_int | e_decl_long | e_decl_longlong);
    case e_decl_unsigned     : return (e_decl_char | e_decl_short | e_decl_int | e_decl_long | e_decl_longlong);
    case e_decl_float        : return 0;
    case e_decl_double       : return (e_decl_long);
    case e_decl_void         : return 0;
    case e_decl_auto         : return 0;
    case e_decl_decltype     : return 0;
    case e_decl_user_defined : return 0;
    }
    assert(false);
    return 0;
}
inline int get_decl_specifier_compatibility(e_decl_specifier ds) {
    switch(ds) {
    case e_cv_const             : return ~0 & ~(e_storage_mutable);
    case e_cv_volatile          : return ~0; // compatible with every other specifier and itself
    case e_friend               : return (e_cv_const | e_cv_volatile | e_constexpr);
    case e_typedef              : return (e_cv_const | e_cv_volatile);
    case e_constexpr            : return ~(e_typedef | e_fn_virtual);
    case e_fn_inline            : return (e_cv_const | e_cv_volatile);
    case e_fn_virtual           : return (e_cv_const | e_cv_volatile);
    case e_fn_explicit          : return (e_cv_const | e_cv_volatile);
    case e_storage_register     : return (e_cv_const | e_cv_volatile);
    case e_storage_static       : return (e_cv_const | e_cv_volatile | e_storage_thread_local);
    case e_storage_thread_local : return (e_cv_const | e_cv_volatile | e_storage_extern | e_storage_static);
    case e_storage_extern       : return (e_cv_const | e_cv_volatile | e_storage_thread_local);
    case e_storage_mutable      : return (e_cv_volatile);
    }
    assert(false);
    return 0;
}

struct decl_flags {
    int type_flags = 0;
    int type_compat_flags = ~0;
    int spec_flags = 0;
    int spec_compat_flags = ~0;

    void add(e_decl_type t) {
        if (t == e_decl_long && (type_flags & e_decl_long)) {
            t = e_decl_longlong;
        }

        type_flags |= t;
        type_compat_flags &= get_type_compatibility(t);
    }
    void add(e_decl_specifier ds) {
        spec_flags |= ds;
        spec_compat_flags &= get_decl_specifier_compatibility(ds);
    }

    bool has(e_decl_type dt) const {
        return type_flags & dt;
    }
    bool has(e_decl_specifier ds) const {
        return spec_flags & ds;
    }

    bool is_compatible(e_decl_type t) const {
        if (type_flags & e_decl_long) {
            t = e_decl_longlong;
        }
        return type_compat_flags & t;
    }
    bool is_compatible(e_decl_specifier ds) const {
        return spec_compat_flags & ds;
    }
};

inline e_fundamental_type decl_flags_to_fundamental_type(const decl_flags& flags) {
    if (flags.has(e_decl_user_defined)) {
        return e_fundamental_invalid;
    }

    if (flags.has(e_decl_void)) {
        return e_void;
    }

    if (flags.has(e_decl_bool)) {
        return e_bool;
    }

    // char types
    if (flags.has(e_decl_char16_t)) {
        return e_char16_t;
    }

    if (flags.has(e_decl_char32_t)) {
        return e_char32_t;
    }

    if (flags.has(e_decl_wchar_t)) {
        return e_wchar_t;
    }

    if (flags.has(e_decl_char)) {
        if (flags.has(e_decl_unsigned)) {
            return e_unsigned_char;
        }
        if (flags.has(e_decl_signed)) {
            return e_signed_char;
        }
        return e_char;
    }

    // floating
    if (flags.has(e_decl_float)) {
        return e_float;
    }
    if (flags.has(e_decl_double)) {
        if (flags.has(e_decl_long)) {
            return e_long_double;
        }
        return e_double;
    }

    // integral
    bool is_unsigned = flags.has(e_decl_unsigned);
    if (flags.has(e_decl_longlong)) {
        return is_unsigned ? e_unsigned_long_long_int : e_long_long_int;
    }
    if (flags.has(e_decl_long)) {
        return is_unsigned ? e_unsigned_long_int : e_long_int;
    }
    if (flags.has(e_decl_short)) {
        return is_unsigned ? e_unsigned_short_int : e_short_int;
    }
    if (flags.has(e_decl_int) || flags.has(e_decl_signed) || flags.has(e_decl_unsigned)) {
        return is_unsigned ? e_unsigned_int : e_int;
    }

    // fallback
    return e_fundamental_invalid;
}

// Old stuff
/*
const int type_char                 = 0x0001;
const int type_char16_t             = 0x0002;
const int type_char32_t             = 0x0004;
const int type_wchar_t              = 0x0008;
const int type_bool                 = 0x0010;
const int type_short                = 0x0020;
const int type_int                  = 0x0040;
const int type_long                 = 0x0080;
const int type_longlong             = 0x0100;
const int type_signed               = 0x0200;
const int type_unsigned             = 0x0400;
const int type_float                = 0x0800;
const int type_double               = 0x1000;
const int type_void                 = 0x2000;
const int type_auto                 = 0x4000;
const int type_user_defined         = 0x8000;

const int type_compat_char          = (type_unsigned | type_signed);
const int type_compat_char16_t      = 0;
const int type_compat_char32_t      = 0;
const int type_compat_wchar_t       = 0;
const int type_compat_bool          = 0;
const int type_compat_short         = (type_unsigned | type_signed | type_int);
const int type_compat_int           = (type_unsigned | type_signed | type_short | type_long | type_longlong);
const int type_compat_long          = (type_unsigned | type_signed | type_int | type_double | type_long);
const int type_compat_longlong      = (type_unsigned | type_signed | type_int);
const int type_compat_signed        = (type_char | type_short | type_int | type_long | type_longlong);
const int type_compat_unsigned      = (type_char | type_short | type_int | type_long | type_longlong);
const int type_compat_float         = 0;
const int type_compat_double        = (type_long);
const int type_compat_void          = 0;
const int type_compat_auto          = 0;
const int type_compat_custom        = 0;

const int spec_cv_const             = 0x0001;
const int spec_cv_volatile          = 0x0002;
const int spec_friend               = 0x0004;
const int spec_typedef              = 0x0008;
const int spec_constexpr            = 0x0010;
const int spec_fn_inline            = 0x0020;
const int spec_fn_virtual           = 0x0040;
const int spec_fn_explicit          = 0x0080;
const int spec_storage_register     = 0x0100;
const int spec_storage_static       = 0x0200;
const int spec_storage_thread_local = 0x0400;
const int spec_storage_extern       = 0x0800;
const int spec_storage_mutable      = 0x1000;

const int spec_compat_cv_const               = ~0 & ~(spec_storage_mutable);
const int spec_compat_cv_volatile            = ~0; // compatible with every other specifier and itself
const int spec_compat_friend                 = (spec_cv_const | spec_cv_volatile | spec_constexpr);
const int spec_compat_typedef                = (spec_cv_const | spec_cv_volatile);
const int spec_compat_constexpr              = ~(spec_typedef | spec_fn_virtual);
const int spec_compat_fn_inline              = (spec_cv_const | spec_cv_volatile);
const int spec_compat_fn_virtual             = (spec_cv_const | spec_cv_volatile);
const int spec_compat_fn_explicit            = (spec_cv_const | spec_cv_volatile);
const int spec_compat_storage_register       = (spec_cv_const | spec_cv_volatile);
const int spec_compat_storage_static         = (spec_cv_const | spec_cv_volatile | spec_storage_thread_local);
const int spec_compat_storage_thread_local   = (spec_cv_const | spec_cv_volatile | spec_storage_extern | spec_storage_static);
const int spec_compat_storage_extern         = (spec_cv_const | spec_cv_volatile | spec_storage_thread_local);
const int spec_compat_storage_mutable        = (spec_cv_volatile);
*/