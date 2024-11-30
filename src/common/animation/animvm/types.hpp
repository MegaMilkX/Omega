#pragma once


namespace animvm {
    
    typedef int32_t i32;
    
    enum INSTRUCTION {
        HALT = 0xC0000000,
        PUSH,               // Push a value, next instruction is the value
        POP,
        DPUSH,              // Push a value from the program's data, next instruction is a pointer to that value
        DSTORE,             // put value at sp-1 to an address stored in sp-2
        DLOAD,              // put a value on the stack from address at sp-1
        ADD,
        SUB,
        MUL,
        DIV,
        ADDF,
        SUBF,
        MULF,
        DIVF,
        LT,
        LTE,
        LAND,
        LOR,
        NOT,
        EQ,
        JMP,
        JMPT,
        JMPF,
        JMPLT,
        PRINT,
        PRINTF,
        PRINTH,
        SLEEP,
        HEVT                // Trigger a host event with next instruction as id
    };
    inline std::string instruction_to_str(i32 instr) {
        switch (instr) {
        case HALT: return "HALT";
        case PUSH: return "PUSH";
        case POP: return "POP";
        case DPUSH: return "DPUSH";
        case DSTORE: return "DSTORE";
        case DLOAD: return "DLOAD";
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case ADDF: return "ADDF";
        case SUBF: return "SUBF";
        case MULF: return "MULF";
        case DIVF: return "DIVF";
        case LT: return "LT";
        case LTE: return "LTE";
        case LAND: return "LAND";
        case LOR: return "LOR";
        case NOT: return "NOT";
        case EQ: return "EQ";
        case JMP: return "JMP";
        case JMPT: return "JMPT";
        case JMPF: return "JMPF";
        case JMPLT: return "JMPLT";
        case PRINT: return "PRINT";
        case PRINTF: return "PRINTF";
        case PRINTH: return "PRINTH";
        case SLEEP: return "SLEEP";
        case HEVT: return "HEVT";
        default: return std::to_string(instr);
        }
    }


}