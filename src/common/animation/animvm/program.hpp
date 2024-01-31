#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include <string>
#include "types.hpp"
#include "log/log.hpp"


namespace animvm {

    enum value_type {
        type_void,
        type_float,
        type_int,
        type_bool
    };
    constexpr size_t type_sizes[] = {
        4, 4, 1
    };

    struct vm_variable {
        value_type type;
        i32 addr;
    };

    struct vm_host_event {
        i32 id;
    };

    struct vm_program {
        std::vector<i32> data;
        std::vector<i32> instructions;

        std::map<std::string, vm_variable> variables;
        std::map<std::string, vm_host_event> host_events;

        vm_variable find_variable(const std::string& name) {
            auto it = variables.find(name);
            if (it == variables.end()) {
                return vm_variable{ type_void, -1 };
            }
            return it->second;
        }
        vm_host_event find_host_event(const std::string& name) {
            auto it = host_events.find(name);
            if (it == host_events.end()) {
                return vm_host_event{ -1 };
            }
            return it->second;
        }

        bool decl_variable(value_type t, const std::string& name) {
            if (variables.find(name) != variables.end()) {
                return false;
            }
            int words_to_alloc = type_sizes[t] / sizeof(i32);
            if (type_sizes[t] % sizeof(i32)) {
                ++words_to_alloc;
            }
            vm_variable var;
            var.type = t;
            var.addr = data.size();
            data.resize(data.size() + words_to_alloc);
            variables.insert(std::make_pair(name, var));
            return true;
        }
        bool decl_host_event(const std::string& name, i32 id) {
            if (host_events.find(name) != host_events.end()) {
                return false;
            }
            host_events.insert(std::make_pair(name, vm_host_event{ id }));
            return true;
        }

        void log_instructions() const {
            for (int i = 0; i < instructions.size(); ++i) {
                LOG(instruction_to_str(instructions[i]));
            }
        }
    };

}