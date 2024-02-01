#include "compiler.hpp"

#include "parser.hpp"


namespace animvm {


    int compile(vm_program& prog, const char* source) {
        ast_node node;
        if (!parse(source, node)) {
            return -1;
        }

        int first_instruction = prog.instructions.size();
        try {
            node.make_program(prog);
        } catch(const parse_exception& ex) {
            LOG_ERR("animvm compile error: " << ex.what());
            return -1;
        }

        return first_instruction;
    }


}