#include "parse.hpp"

#include "resolve/resolve.hpp"

#include "type_id/type_id.hpp"

ast_node eat_translation_unit_2(parse_state& ps) {
    ast_node n = ast_node::null();
    if (ps.is_limited()) {
        n = eat_declaration_seq_limited_2(ps);
    } else {
        n = eat_declaration_seq_2(ps);
    }

    if (ps.peek_token().type != tt_eof) {
        // NOTE: This can only trigger in unlimited mode
        throw parse_exception("Failed to reach end of file (unknown syntax)", ps.peek_token());
    }

    // TODO: Should not be here
    n.dbg_print();
    printf("\n\n");

    //resolve(ps, n);

    dbg_printf_color("Symbol table dump:\n", DBG_WHITE);
    ps.get_root_scope()->print_all();
    printf("\n");

    dbg_printf_color("type-id dump:\n", DBG_WHITE);
    type_id_storage::get()->dbg_print_all();
    printf("\n");

    return n;
}

