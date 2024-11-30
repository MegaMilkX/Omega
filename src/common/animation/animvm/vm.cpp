#include "vm.hpp"


namespace animvm {

    VM_ERR VM::exec() {
        switch (prog[pc]) {
        case HALT:
            running = false;
            break;
        case PUSH:
            fetch();
            if (sp >= VM_STACK_CAP) {
                return VM_STACK_OVERFLOW;
            }
            mem_stack[sp] = prog[pc];
            ++sp;
            break;
        case POP:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            --sp;
            break;
        case DPUSH:
            fetch();
            if (sp >= VM_STACK_CAP) {
                return VM_STACK_OVERFLOW;
            }
            mem_stack[sp] = prog_data[prog[pc]];
            ++sp;
            break;
        case DSTORE:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            prog_data[mem_stack[sp - 2]] = mem_stack[sp - 1];
            --sp;
            break;
        case DLOAD:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 1] = prog_data[mem_stack[sp - 1]];
            break;
        case ADD: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] + mem_stack[sp - 1];
            --sp;
            break;
        }
        case SUB: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] - mem_stack[sp - 1];
            --sp;
            break;
        }
        case MUL: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] * mem_stack[sp - 1];
            --sp;
            break;
        }
        case DIV: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] / mem_stack[sp - 1];
            --sp;
            break;
        }
        case ADDF: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            *(float*)&mem_stack[sp - 2] = *(float*)&mem_stack[sp - 2] + *(float*)&mem_stack[sp - 1];
            --sp;
            break;
        }
        case SUBF: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            *(float*)&mem_stack[sp - 2] = *(float*)&mem_stack[sp - 2] - *(float*)&mem_stack[sp - 1];
            --sp;
            break;
        }
        case MULF: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            *(float*)&mem_stack[sp - 2] = *(float*)&mem_stack[sp - 2] * *(float*)&mem_stack[sp - 1];
            --sp;
            break;
        }
        case DIVF: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            *(float*)&mem_stack[sp - 2] = *(float*)&mem_stack[sp - 2] / *(float*)&mem_stack[sp - 1];
            --sp;
            break;
        }
        case LT:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] < mem_stack[sp - 1];
            --sp;
            break;
        case LTE:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] <= mem_stack[sp - 1];
            --sp;
            break;
        case LAND:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] && mem_stack[sp - 1];
            --sp;
            break;
        case LOR:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = mem_stack[sp - 2] || mem_stack[sp - 1];
            --sp;
            break;
        case NOT:
            if (sp < 1) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 1] = !mem_stack[sp - 1];
            break;
        case EQ:
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            mem_stack[sp - 2] = !(mem_stack[sp - 2] - mem_stack[sp - 1]);
            --sp;
            break;
        case JMP:
            fetch();
            pc = prog[pc] - 1;
            break;
        case JMPT:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            fetch();
            if (mem_stack[sp - 1] != 0) {
                pc = prog[pc] - 1;
            }
            break;
        case JMPF:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            fetch();
            if (mem_stack[sp - 1] == 0) {
                pc = prog[pc] - 1;
            }
            break;
        case JMPLT: {
            if (sp < 2) {
                return VM_STACK_UNDERFLOW;
            }
            fetch();
            if (mem_stack[sp - 2] < mem_stack[sp - 1]) {
                pc = prog[pc] - 1;
            }
            break;
        }
        case PRINT:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            LOG("ANIMVM: " << mem_stack[sp - 1]);
            break;
        case PRINTF:
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            LOG("ANIMVM: " << *(float*)&mem_stack[sp - 1]);
            break;
        case PRINTH: {
            if (sp == 0) {
                return VM_STACK_UNDERFLOW;
            }
            i32 p = mem_stack[sp - 1];
            LOG("ANIMVM: " << &((uint8_t*)mem_heap)[p]);
            --sp;
            break;
        }
        case SLEEP: {
            fetch();
            std::this_thread::sleep_for(std::chrono::milliseconds(prog[pc]));
            break;
        }
        case HEVT: {
            fetch();
            do_host_event(prog[pc]);
            break;
        }
        default:
            return VM_INVALID_INSTRUCTION;
        }
        return VM_OK;
    }

}