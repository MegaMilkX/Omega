#pragma once

#include <algorithm>
#include <unordered_map>
#include <memory>
#include <format>
#include <functional>
#include <fstream>

#include "log/log.hpp"
#include "event/event.hpp"
#include "console_command.hpp"


class ConBase {
public:
    std::string name;
    std::string desc;

    ConBase(std::string name, std::string desc)
        : name(name), desc(desc) {}
    virtual ~ConBase() = default;
};

class ConGroup : public ConBase {
public:
    std::unordered_map<std::string, std::unique_ptr<ConBase>> entries;

    ConGroup(std::string name, std::string desc)
        : ConBase(name, desc) {}
};

class ConFloat : public ConBase {
    float value;
    float default_;
    float min_, max_;
public:
    ConFloat(std::string name, std::string desc, float default_, float min_ = -FLT_MAX, float max_ = FLT_MAX)
        : ConBase(name, desc), value(default_), default_(default_), min_(min_), max_(max_) {}
    float get() const { return value; }
    float get_min() const { return min_; }
    float get_max() const { return max_; }
    void set(float v) {
        value = std::clamp(v, min_, max_);
        on_change.invoke(value);
    }

    Event<float> on_change;
};

class ConInt : public ConBase {
    int value;
    int default_;
    int min_, max_;
public:
    ConInt(std::string name, std::string desc, int default_, int min_ = -INT_MAX, int max_ = INT_MAX)
        : ConBase(name, desc), value(default_), default_(default_), min_(min_), max_(max_) {}
    int get() const { return value; }
    int get_min() const { return min_; }
    int get_max() const { return max_; }
    void set(int v) {
        value = std::clamp(v, min_, max_);
        on_change.invoke(value);
    }

    Event<int> on_change;
};

class ConBool : public ConBase {
    bool value;
    bool default_;
public:
    ConBool(std::string name, std::string desc, bool default_)
        : ConBase(name, desc), value(default_), default_(default_) {}
    bool get() const { return value; }
    void set(int v) {
        value = v;
        on_change.invoke(value);
    }

    Event<bool> on_change;
};

class ConCmd : public ConBase {
public:
    ConCmd(std::string name, std::string desc, std::function<void(const ConsoleCommand&)> handler)
        : ConBase(name, desc), handler(handler) {}
    std::function<void(const ConsoleCommand&)> handler;
};

class ConRegistry {
    std::unordered_map<std::string, std::unique_ptr<ConBase>> entries;

    std::vector<std::string_view> splitConPath(std::string_view path) {
        std::vector<std::string_view> parts;
        while (true) {
            auto dot = path.find('.');
            parts.push_back(path.substr(0, dot));
            if(dot == std::string_view::npos) break;
            path.remove_prefix(dot + 1);
        }
        return parts;
    }
    std::string listEntries(const std::unordered_map<std::string, std::unique_ptr<ConBase>>& entries) {
        int max_cmd_name_size = 0;
        for (auto& kv : entries) {
            max_cmd_name_size = gfxm::_max(max_cmd_name_size, int(kv.second->name.size()));
        }
        std::string output;
        for (auto& kv : entries) {
            int indent = (max_cmd_name_size - int(kv.second->name.size())) + 1;
            std::string sindent;
            for (int i = 0; i < indent; ++i) {
                sindent += " ";
            }
            if (auto* grp = dynamic_cast<ConGroup*>(kv.second.get())) {
                output += std::format("- {}:{}Group of {} entries\n", kv.second->name, sindent, grp->entries.size());
            } else if(auto* con_float = dynamic_cast<ConFloat*>(kv.second.get())) {
                std::string smin = con_float->get_min() != -FLT_MAX ? std::format("{:.2f}", con_float->get_min()) : std::string("~");
                std::string smax = con_float->get_max() != FLT_MAX ? std::format("{:.2f}", con_float->get_max()) : std::string("~");
                output += std::format("- {}:{}[float][{}, {}] {}\n", kv.second->name, sindent, smin, smax, kv.second->desc);
            } else if(auto* con_int = dynamic_cast<ConInt*>(kv.second.get())) {
                std::string smin = con_int->get_min() != -INT_MAX ? std::format("{}", con_int->get_min()) : "~";
                std::string smax = con_int->get_max() != INT_MAX ? std::format("{}", con_int->get_max()) : "~";
                output += std::format("- {}:{}[int][{}, {}] {}\n", kv.second->name, sindent, smin, smax, kv.second->desc);
            } else if(auto* con_bool = dynamic_cast<ConBool*>(kv.second.get())) {
                output += std::format("- {}:{}[bool] {}\n", kv.second->name, sindent, kv.second->desc);
            } else {
                output += std::format("- {}:{}{}\n", kv.second->name, sindent, kv.second->desc);
            }
        }
        return output;
    }
    ConBase* findEntry(const std::string& path) {
        auto parts = splitConPath(path);
        auto* entry_map = &entries;
        for (int i = 0; i < parts.size() - 1; ++i) {
            std::string part(parts[i]);
            for (int i = 0; i < part.size(); ++i) {
                part[i] = std::tolower(part[i]);
            }

            auto it = entry_map->find(std::string(part));
            if (it == entry_map->end()) {
                LOG_WARN("Can't find group '" << part << "'");
                return nullptr;
            }
            ConGroup* grp = dynamic_cast<ConGroup*>(it->second.get());
            if (!grp) {
                LOG_WARN("'" << part << "' is not a group");
                return nullptr;
            }
            entry_map = &grp->entries;
        }

        std::string lcl_name(parts.back());
        for (int i = 0; i < lcl_name.size(); ++i) {
            lcl_name[i] = std::tolower(lcl_name[i]);
        }
        auto it = entry_map->find(lcl_name);
        if (it == entry_map->end()) {
            LOG_WARN("No such command, variable or group: '" << path << "'");
            return nullptr;
        }

        return it->second.get();
    }
public:
    class WatchTicket {
        friend ConRegistry;
        std::string var;
        uint32_t handler_id;
    public:
        WatchTicket()
        : var(""), handler_id(0) {}
        WatchTicket(const std::string& var, uint32_t handler_id)
        : var(var), handler_id(handler_id) {}
    };

    static ConRegistry* get() {
        static ConRegistry conreg;
        return &conreg;
    }

    ConRegistry() {
        registerCmd("help", "displays a list of top level commands, variables and groups", [this](const ConsoleCommand& cmd) {
            std::string output("Available commands: \n\n");
            output += listEntries(entries);
            LOG(output);
        });
    }

    void register_(std::unique_ptr<ConBase> entry) {
        auto parts = splitConPath(entry->name);
        auto* entry_map = &entries;
        for (int i = 0; i < parts.size() - 1; ++i) {
            std::string part(parts[i]);
            for (int i = 0; i < part.size(); ++i) {
                part[i] = std::tolower(part[i]);
            }

            if (part.empty()) {
                LOG_ERR("Can't register command or var, invalid name: '" << entry->name << "'");
                return;
            }

            auto it = entry_map->find(std::string(part));
            if(it != entry_map->end()) {
                if (ConGroup* grp = dynamic_cast<ConGroup*>(it->second.get())) {
                    entry_map = &grp->entries;
                    continue;
                } else {
                    LOG_ERR("'" << part << "' is not a con group");
                    return;
                }
            } else {
                std::unique_ptr<ConGroup> grp = std::make_unique<ConGroup>(std::string(part), "");
                auto next_entry_map = &grp->entries;
                entry_map->operator[](std::string(part)) = std::move(grp);
                entry_map = next_entry_map;
            }
        }
        entry_map->operator[](std::string(parts.back())) = std::move(entry);
    }
    void registerCmd(const std::string& name, const std::string& desc, std::function<void(const ConsoleCommand&)> fn) {
        register_(std::make_unique<ConCmd>(name, desc, fn));
    }
    void registerFloat(const std::string& name, const std::string& desc, float default_, float min_ = -FLT_MAX, float max_ = FLT_MAX) {
        register_(std::make_unique<ConFloat>(name, desc, default_, min_, max_));
    }
    void registerInt(const std::string& name, const std::string& desc, int default_, int min_ = -INT_MAX, int max_ = INT_MAX) {
        register_(std::make_unique<ConInt>(name, desc, default_, min_, max_));
    }
    void registerBool(const std::string& name, const std::string& desc, bool default_) {
        register_(std::make_unique<ConBool>(name, desc, default_));
    }
    bool dispatch(const ConsoleCommand& cmd) {
        ConBase* entry = findEntry(std::string(cmd.name()));
        if (!entry) {
            return false;
        }

        if (auto* var = dynamic_cast<ConFloat*>(entry)) {
            if (cmd.arg_count() == 0) {
                LOG(var->name << " == " << var->get());
            } else {
                var->set(cmd.arg<float>(.0f));
                LOG(var->name << " set to " << var->get());
            }
            return true;
        } else if (auto* var = dynamic_cast<ConInt*>(entry)) {
            if (cmd.arg_count() == 0) {
                LOG(var->name << " == " << var->get());
            } else {
                var->set(cmd.arg<int>(0));
                LOG(var->name << " set to " << var->get());
            }
            return true;
        } else if (auto* var = dynamic_cast<ConBool*>(entry)) {
            if (cmd.arg_count() == 0) {
                LOG(var->name << " == " << var->get());
            } else {
                var->set(cmd.arg<int>(0));
                LOG(var->name << " set to " << var->get());
            }
            return true;
        } else if (auto* command = dynamic_cast<ConCmd*>(entry)) {
            command->handler(cmd);
            return true;
        } else if (auto* grp = dynamic_cast<ConGroup*>(entry)) {
            std::string output = std::format("'{}' entries:\n\n", grp->name);
            output += listEntries(grp->entries);
            LOG(output);
            return true;
        }

        LOG_WARN("Unreachable code while dispatching cmd: '" << cmd.name() << "'");
        return false;
    }
    
    ConFloat* getFloatVar(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConFloat*>(entry)) {
            return e;
        }
        LOG_ERR("getFloat(): " << var << " is not a float convar");
        return nullptr;
    }
    ConInt* getIntVar(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConInt*>(entry)) {
            return e;
        }
        LOG_ERR("getInt(): " << var << " is not an int convar");
        return nullptr;
    }
    ConBool* getBoolVar(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConBool*>(entry)) {
            return e;
        }
        LOG_ERR("getBool(): " << var << " is not a bool convar");
        return nullptr;
    }
    float getFloat(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConFloat*>(entry)) {
            return e->get();
        }
        LOG_ERR("getFloat(): " << var << " is not a float convar");
        return .0f;
    }
    int getInt(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConInt*>(entry)) {
            return e->get();
        }
        LOG_ERR("getInt(): " << var << " is not an int convar");
        return 0;
    }
    int getBool(const std::string& var) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConBool*>(entry)) {
            return e->get();
        }
        LOG_ERR("getBool(): " << var << " is not a bool convar");
        return false;
    }
    WatchTicket watchFloat(const std::string& var, Event<float>::handler_t hdl) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConFloat*>(entry)) {
            auto id = e->on_change.subscribe(hdl);
            e->on_change.invoke_one(id, e->get());
            return WatchTicket(var, id);
        }
        LOG_ERR("watchFloat(): " << var << " is not a float convar");
        return WatchTicket();
    }
    WatchTicket watchInt(const std::string& var, Event<int>::handler_t hdl) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConInt*>(entry)) {
            auto id = e->on_change.subscribe(hdl);
            e->on_change.invoke_one(id, e->get());
            return WatchTicket(var, id);
        }
        LOG_ERR("watchInt(): " << var << " is not an int convar");
        return WatchTicket();
    }
    WatchTicket watchBool(const std::string& var, Event<bool>::handler_t hdl) {
        ConBase* entry = findEntry(var);
        if (auto* e = dynamic_cast<ConBool*>(entry)) {
            auto id = e->on_change.subscribe(hdl);
            e->on_change.invoke_one(id, e->get());
            return WatchTicket(var, id);
        }
        LOG_ERR("watchBool(): " << var << " is not a bool convar");
        return WatchTicket();
    }
    void unwatch(const WatchTicket& t) {
        ConBase* entry = findEntry(t.var);
        if (!entry) {
            LOG_ERR("Tried to unwatch a nonexisting convar: '" << t.var << "'");
            return;
        }
        if (auto* e = dynamic_cast<ConFloat*>(entry)) {
            e->on_change.unsubscribe(t.handler_id);
        } else if (auto* e = dynamic_cast<ConInt*>(entry)) {
            e->on_change.unsubscribe(t.handler_id);
        } else if (auto* e = dynamic_cast<ConBool*>(entry)) {
            e->on_change.unsubscribe(t.handler_id);
        }
    }

    bool runFile(const std::string& path) {
        std::ifstream file(path);
        if (!file) {
            LOG_WARN("File not found: '" << path << "'");
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            auto comment = line.find("//");
            if(comment != std::string::npos) {
                line.erase(comment);
            }
            if (line.find_first_not_of(" \t\r") == std::string::npos) {
                continue;
            }
            dispatch(ConsoleCommand(line));
        }

        return true;
    }
};

