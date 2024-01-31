#pragma once

#include "parser.hpp"
#include "vm.hpp"
#include "compiler.hpp"


namespace animvm {

    inline void host_event_cb(int32_t id) {
        switch (id) {
        case 1:
            //Beep(150, 300);
            LOG("Host event: " << id);
            break;
        case 2:
            //Beep(200, 300);
            LOG("Host event: " << id);
            break;
        case 3:
            //Beep(250, 300);
            LOG("Host event: " << id);
            break;
        case 4:
            //Beep(300, 300);
            LOG("Host event: " << id);
            break;
        default:
            LOG_WARN("Unknown anim host event");
        }
    }
    
    inline bool expr_parse(const char* expr) {
        try {
            parse_state ps(expr);

            ast_node node;
            node.type = ast_type::ast_translation_unit;
            node.translation_unit = new ast_translation_unit;
            while (!ps.is_eof()) {
                ps.push_rewind();

                node.translation_unit->statements.push_back(ast_node());
                parse_statement(ps, node.translation_unit->statements.back());

                if (ps.count_read() == 0) {
                    LOG_ERR("Failed to parse animvm program!");
                    break;
                }

                auto tokens = ps.pop_rewind_get_tokens_read();
                std::string read;
                for (auto& tok : tokens) {
                    read += tok.str;
                    read += " ";
                }
                LOG(read);
            }
            
            vm_program prog;
            
            prog.decl_host_event("anim_end", 1);
            prog.decl_host_event("shoot", 2);
            prog.decl_host_event("footstep", 3);
            prog.decl_host_event("fevt_door_open_end", 4);
            
            node.make_program(prog);
            node.free();

            prog.instructions.push_back(HALT);
            prog.log_instructions();
            
            VM vm;
            vm.set_host_event_cb(&host_event_cb);
            vm.load_program(&prog);
            i32 ret = vm.run();
            LOG("Anim VM returned(float): " << *(float*)&ret);
            LOG("Anim VM returned(int): " << ret);

        } catch(const parse_exception& ex) {
            LOG_ERR("anim expression error: " << ex.what());
            return false;
        }
        return true;
    }
    
    inline void vm_test() {
        float fa = 2.0f;
        float fb = 0.1f;
        
        vm_program program;
        program.instructions = {
            PUSH, *(i32*)&fa,
            PUSH, *(i32*)&fb,
            SUBF,
            PUSH, 0,
            JMPLT, 13,
            POP,
            PRINTF,
            //SLEEP, 500,
            JMP, 2,
            POP,
            PRINTF,
            PUSH, 0,
            PRINTH,
            HALT
        };

        VM vm;
        vm.set_heap_string(0, "Hello, World!");
        vm.load_program(&program);
        i32 ret = vm.run();
        LOG("Anim VM finished with: " << ret);
    }

}