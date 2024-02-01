#pragma once

#include <assert.h>
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
        0, 4, 4, 1
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

        void clear() {
            data.clear();
            instructions.clear();
            variables.clear();
            host_events.clear();
        }

        void set_variable_float(i32 addr, float value) {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return;
            }
            data[addr] = *(i32*)&value;
        }
        void set_variable_int(i32 addr, i32 value) {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return;
            }
            data[addr] = value;
        }
        void set_variable_bool(i32 addr, bool value) {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return;
            }
            data[addr] = value;
        }
        float get_variable_float(i32 addr) const {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return .0f;
            }
            return *(float*)&data[addr];
        }
        int get_variable_int(i32 addr) const {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return 0;
            }
            return data[addr];
        }
        bool get_variable_bool(i32 addr) const {
            if (addr < 0 || addr >= data.size()) {
                assert(false);
                return false;
            }
            return data[addr];
        }

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

        // returns the address
        int decl_variable(value_type t, const std::string& name) {
            if (variables.find(name) != variables.end()) {
                return -1;
            }
            volatile int words_to_alloc = type_sizes[t] / sizeof(i32);
            if (type_sizes[t] % sizeof(i32)) {
                ++words_to_alloc;
            }
            vm_variable var;
            var.type = t;
            var.addr = data.size();
            data.resize(data.size() + words_to_alloc);
            variables.insert(std::make_pair(name, var));
            return var.addr;
        }
        // id - pass -1 to get an automatically assigned id
        int decl_host_event(const std::string& name, i32 id) {
            if (host_events.find(name) != host_events.end()) {
                return -1;
            }
            if (id = -1) {
                id = host_events.size();
            }
            host_events.insert(std::make_pair(name, vm_host_event{ id }));
            return id;
        }

        void log_instructions() const {
            for (int i = 0; i < instructions.size(); ++i) {
                LOG(instruction_to_str(instructions[i]));
            }
        }
    };

}