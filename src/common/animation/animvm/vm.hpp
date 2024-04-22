#pragma once

#include <assert.h>
#include <conio.h>
#include <functional>
#include "program.hpp"
#include "log/log.hpp"


namespace animvm {

    enum VM_ERR {
        VM_OK,
        VM_INVALID_INSTRUCTION,
        VM_STACK_OVERFLOW,
        VM_STACK_UNDERFLOW
    };

    constexpr int VM_STACK_CAP = 512;
    constexpr int VM_HEAP_CAP = 512;
    class VM {
        i32 mem_stack[VM_STACK_CAP];
        int sp = 0;
        i32 mem_heap[VM_HEAP_CAP];
        vm_program* program = 0;
        i32* prog_data = 0;
        i32* prog = 0;
        int prog_len = 0;
        int pc = -1;
        bool running = false;

        std::function<void(i32)> fn_host_event_cb;

        void fetch() {
            ++pc;
        }
        VM_ERR exec();

        void do_host_event(i32 id) {
            if (fn_host_event_cb) {
                fn_host_event_cb(id);
            }
        }
    public:
        void set_host_event_cb(std::function<void(i32)> fn) {
            fn_host_event_cb = fn;
        }

        void set_heap_string(i32 addr, const char* str) {
            size_t len = strlen(str) + 1;
            i32 byte_addr = addr * sizeof(mem_heap[0]);
            uint8_t* begin = ((uint8_t*)mem_heap) + byte_addr;
            i32 bytes_available = VM_HEAP_CAP * sizeof(mem_heap[0]) - byte_addr;
            for (int i = 0; i < len && i < bytes_available; ++i) {
                begin[i] = str[i];
            }
        }

        void clear_stack() {
            sp = 0;
        }

        void load_program(vm_program* p) {
            program = p;
            prog_data = p->data.data();
            prog = p->instructions.data();
            prog_len = p->instructions.size();
        }

        i32 run() {
            return run_at(0);
        }

        i32 run_at(i32 addr) {
            if (!prog || !prog_len) {
                assert(false);
                return -1;
            }

            pc = addr - 1;

            running = true;
            while (running) {
                fetch();

                // NOTE: Kept for debugging
                //int begin_pc = pc;

                VM_ERR err = exec();
                if (err != VM_OK) {
                    LOG_ERR("anim VM error: " << err);
                    LOG_ERR("instruction: " << instruction_to_str(prog[pc]));
                    LOG_ERR("stack pointer: " << sp);
                    LOG_ERR("program cursor: " << pc);
                    assert(false);
                    break;
                }


                // NOTE: Kept for debugging
                /*
                int end_pc = pc;

                LOG("exec: ");
                for (int i = begin_pc; i <= end_pc; ++i) {
                    LOG(instruction_to_str(prog[i]));
                }
                LOG("stack: ");
                for (int i = 0; i < sp; ++i) {
                    LOG(mem_stack[i]);
                }
                getch();*/
            }
            return mem_stack[sp - 1];
        }        
    };

}
