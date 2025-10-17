#include "type_id.hpp"

#include "symbol.hpp"


void type_id_2_member_pointer::build_source_name(std::string& inout) const {
    std::string c_part = is_const ? std::string(" ") + cv_to_string(cv_const) : "";
    std::string v_part = is_volatile ? std::string(" ") + cv_to_string(cv_volatile) : "";

    if (get_next()->is_function()) {
        inout = "(" + owner->get_source_name() + "::*" + c_part + v_part + inout + ")";
    } else {
        inout = owner->get_source_name() + "::*" + c_part + v_part + inout;
    }
    get_next()->build_source_name(inout);
}

void type_id_2_user_type::build_source_name(std::string& inout) const {
    std::vector<std::string> parts;
    parts.reserve(3);

    if(is_const) parts.push_back(cv_to_string(cv_const));
    if(is_volatile) parts.push_back(cv_to_string(cv_volatile));
    parts.push_back(sym->global_qualified_name);

    std::string str;
    for (int i = 0; i < parts.size(); ++i) {
        if(i) str += " ";
        str += parts[i];
    }

    inout = str + inout;
}


type_id_graph_node* type_id_storage::get_base_node(symbol_ref sym, bool is_const, bool is_volatile) {
    std::string name = sym->get_internal_name();
    if (is_const) {
        name = "K" + name;
    }
    if (is_volatile) {
        name = "V" + name;
    }
    type_id_graph_node* n = find_base_node(name.c_str());
    if (n) {
        return n;
    }
    n = new type_id_graph_node;
    base_nodes.insert(std::make_pair(std::string(name), std::unique_ptr<type_id_graph_node>(n)));
    n->type_id.reset(new type_id_2_user_type(name, sym, is_const, is_volatile));
    n->type_id->lookup_node = n;
    return n;
}


type_id_graph_node* type_id_storage::walk_to_member_ptr(type_id_graph_node* current, symbol_ref owner, bool is_const, bool is_volatile) {
    std::string sub_name = "M" + owner->get_internal_name();
    if (is_const) {
        sub_name = "K" + sub_name;
    }
    if (is_volatile) {
        sub_name = "V" + sub_name;
    }

    type_id_graph_node* n = current->walk(sub_name);
    if (n->type_id) {
        return n;
    }

    type_id_2_member_pointer* tid = new type_id_2_member_pointer(current->type_id.get(), sub_name + current->type_id->get_internal_name(), owner, is_const, is_volatile);
    n->type_id.reset(tid);
    n->type_id->lookup_node = n;
    return n;
}

