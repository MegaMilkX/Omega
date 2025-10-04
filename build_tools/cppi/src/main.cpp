
#include "preprocessor/preprocessor.hpp"

#include "parse_state.hpp"

#include "parsing/declaration.hpp"
#include "parsing/template_declaration.hpp"
#include "parse_exception.hpp"

#include "parsing/attribute_specifier.hpp"
#include "common.hpp"
#include "parsing/class_specifier.hpp"
#include "parsing/enum_specifier.hpp"
#include "parsing/namespace.hpp"
#include "parsing/type_specifier.hpp"

#include "parse2/parse.hpp"

#include "lib/popl.hpp"
#include "lib/inja.hpp"

#include "filesystem/filesystem.hpp"

#define PARSE_LIMITED true


bool eat_translation_unit(parse_state& ps) {
    while (true) {
        if (eat_declaration(ps)) {
            continue;
        }
        if (eat_template_declaration(ps)) {
            continue;
        }
        break;
    }
    return true;
}

bool eat_cppi_block(parse_state& ps, attribute_specifier& attr_spec) {
    while (true) {
        if (eat_declaration(ps, true)) {
            continue;
        }
        if (eat_template_declaration(ps)) {
            continue;
        }
        attribute_specifier attr_spec_end;
        if (eat_attribute_specifier(ps, attr_spec_end)) {
            if (attr_spec_end.find_attrib("cppi_end")) {
                expect(ps, ";");
                ps.no_reflect = false;
                break;
            }
        }

        throw parse_exception("Reached an end of file or an unsupported construct before [[cppi_end]]", ps.peek_token());
    }
    return true;
}


bool eat_translation_unit_limited(parse_state& ps) {
    while (ps.peek_token().type != tt_eof) {
        attribute_specifier attr_spec;
        if (eat_namespace_definition(ps, true)) {
            continue;
        } else if (eat_attribute_specifier_seq(ps, attr_spec)) {
            if (attr_spec.find_attrib("cppi_begin")) {
                if (attr_spec.find_attrib("no_reflect")) {
                    ps.no_reflect = true;
                }
                expect(ps, ";");
                eat_cppi_block(ps, attr_spec);
                continue;
            } else if (attr_spec.find_attrib("cppi_class")) {
                expect(ps, ";");
                if (!eat_class_specifier_limited(ps, attr_spec)) {
                    throw parse_exception("cppi_class attribute must be followed by a class definition", ps.latest_token);
                }
                continue;
            } else if(attr_spec.find_attrib("cppi_enum")) {
                expect(ps, ";");
                if (!eat_enum_specifier_limited(ps)) {
                    throw parse_exception("cppi_enum attribute must be followed by an enumeration definition", ps.latest_token);
                }
                continue;
            }

            continue;
        }

        if (eat_balanced_token(ps)) {
            continue;
        }
    }

    return true;
}

struct cache_data {
    std::string cmd_cache;
    std::vector<std::string> source_files;

    std::unordered_map<std::string, time_t> timestamps;
    std::unordered_map<std::string, std::vector<std::string>> dependant_sets;

    void set_source_files(const std::vector<std::string>& sources) {
        source_files = sources;
    }
};

std::string make_cmd_cache(int argc, char* argv[]) {
    std::string cmd_cache;
    for (int i = 0; i < argc; ++i) {
        cmd_cache += argv[i];
    }
    return cmd_cache;
}

bool write_cache_data(const cache_data& cache) {
    nlohmann::json json;
    json["cmd_cache"] = cache.cmd_cache;
    
    nlohmann::json& jsources = json["sources"];
    for (int i = 0; i < cache.source_files.size(); ++i) {
        jsources[i] = cache.source_files[i];
    }

    nlohmann::json& jdependants = json["dependants"];
    for (auto& it : cache.dependant_sets) {
        auto& dependency_name = it.first;
        nlohmann::json& jarray = jdependants[dependency_name];
        int i = 0;
        for (auto& dep : it.second) {
            jarray[i] = dep;
            ++i;
        }
    }

    nlohmann::json& jtimestamps = json["timestamps"];
    for (auto& it : cache.timestamps) {
        jtimestamps[it.first] = it.second;
    }

    FILE* f = fopen("cppi_cache.json", "wb");
    if (!f) {
        return false;
    }
    std::string dump = json.dump(4);
    fwrite(dump.data(), dump.size(), 1, f);
    fclose(f);

    printf("Cache file updated\n");
    return true;
}
bool read_cache_data(cache_data& cache) {
    FILE* f = fopen("cppi_cache.json", "rb");
    if (!f) {
        return false;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0) {
        fclose(f);
        return false;
    }

    std::string buf;
    buf.resize(fsize);
    fread((void*)buf.data(), buf.size(), 1, f);
    fclose(f);

    nlohmann::json json = nlohmann::json::parse(buf);
    cache.cmd_cache = json["cmd_cache"].get<std::string>();

    nlohmann::json& jsources = json["sources"];
    for (auto& jsrc : jsources) {
        cache.source_files.push_back(jsrc.get<std::string>());
    }

    nlohmann::json& jtimestamps = json["timestamps"];
    for (auto& kv : jtimestamps.items()) {
        cache.timestamps[kv.key()] = kv.value().get<time_t>();
    }

    nlohmann::json& jdependants = json["dependants"];
    for (auto& kv : jdependants.items()) {
        nlohmann::json& jdependant_list = jdependants[kv.key()];
        auto& dependant_list = cache.dependant_sets[kv.key()];
        for (auto& kv2 : jdependant_list) {
            dependant_list.push_back(kv2.get<std::string>());
        }
    }

    printf("Cache file read successfully\n");
    return true;
}

void find_dependent_sources(
    cache_data& cache,
    const std::unordered_set<std::string>& source_file_set,
    const std::string& changed_file,
    std::unordered_set<std::string>& filtered_sources
) {
    {
        auto it = source_file_set.find(changed_file);
        if (it != source_file_set.end()) {
            filtered_sources.insert(*it);
            return;
        }
    }

    auto& it = cache.dependant_sets.find(changed_file);
    if (it == cache.dependant_sets.end()) {
        return;
    }
    for (auto& dep : it->second) {
        find_dependent_sources(cache, source_file_set, dep, filtered_sources);
    }
}

struct translation_unit {
    std::unique_ptr<preprocessor> pp;
    std::unique_ptr<parse_state> ps;
};

nlohmann::json function_overload_to_json(symbol_func_overload* func, std::shared_ptr<symbol>& owner) {
    nlohmann::json j;
    j["NAME"] = func->name;
    j["QUALIFIED_NAME"] = func->global_qualified_name;
    auto type_id_ = func->type_id_;
    if (owner) {
        auto member_ptr = type_id_.push_front<type_id_part_member_ptr>();
        member_ptr->owner = owner;
    } else {
        auto ptr = type_id_.push_front<type_id_part_ptr>();
    }
    j["SIGNATURE"] = type_id_.make_string();
    return j;
}

void make_reflection_template_data(std::shared_ptr<symbol>& sym, nlohmann::json& json);

void make_reflection_json_member_object(symbol_object* sym_obj, nlohmann::json& json) {
    printf("'%s': %s\n", sym_obj->file->filename_canonical.c_str(), sym_obj->global_qualified_name.c_str());

    auto& jobject = json.emplace_back();
    jobject["DECL_NAME"] = sym_obj->global_qualified_name;
    jobject["ALIAS"] = sym_obj->name;
    jobject["NAME"] = sym_obj->name;
    jobject["DECL_TYPE"] = sym_obj->type_id_.make_string();
}
void make_reflection_json_member_function(
    symbol_ref sym_func,
    symbol_ref owner,
    nlohmann::json& jclass,
    nlohmann::json& jfunctions,
    nlohmann::json& jprops
) {
    printf("'%s': %s\n", sym_func->file->filename_canonical.c_str(), sym_func->global_qualified_name.c_str());

    symbol_function* func = (symbol_function*)sym_func.get();
    for (auto& overload : func->overloads) {
        attribute* attrib_get = overload->attrib_spec.find_attrib("get");
        attribute* attrib_set = overload->attrib_spec.find_attrib("set");
        attribute* attrib_serialize_json = overload->attrib_spec.find_attrib("serialize_json");
        attribute* attrib_deserialize_json = overload->attrib_spec.find_attrib("deserialize_json");
        if (attrib_get) {
            assert(attrib_get->value.is_string());
            std::string prop_name = attrib_get->value.get_string();
            jprops[prop_name]["get"] = function_overload_to_json(overload.get(), owner);

            type_id tid = overload->type_id_.get_return_type();
            jprops[prop_name]["type"] = tid.make_string(true, true, true);
        }
        if (attrib_set) {
            assert(attrib_set->value.is_string());
            std::string prop_name = attrib_set->value.get_string();
            jprops[prop_name]["set"] = function_overload_to_json(overload.get(), owner);

            type_id tid = overload->type_id_.get_return_type();
            jprops[prop_name]["type"] = tid.make_string(true, true, true);
        }
        if (attrib_serialize_json) {
            jclass["SERIALIZE_JSON_FN"] = func->name;
        }
        if (attrib_deserialize_json) {
            jclass["DESERIALIZE_JSON_FN"] = func->name;
        }
    }
}
void make_reflection_json_class(symbol_ref sym_class_ref, nlohmann::json& json) {
    symbol_class* sym_class = dynamic_cast<symbol_class*>(sym_class_ref.get());
    printf("CLASS %s\n", sym_class->global_qualified_name.c_str());

    std::string alt_name = sym_class->global_qualified_name;
    std::replace(alt_name.begin(), alt_name.end(), ':', '_');

    json["HEADER_NAME"] = sym_class->file->file_name;
    auto& jclasses = json["CLASSES"];
    auto& jclass = jclasses[alt_name];// [sym->global_qualified_name];
    auto& jobjects = jclass["OBJECTS"] = nlohmann::json::array();
    auto& jfunctions = jclass["FUNCTIONS"] = nlohmann::json::array();
    auto& jprops = jclass["PROPS"] = nlohmann::json::object();
    jclass["DECL_NAME"] = sym_class->global_qualified_name;
    jclass["ALT_NAME"] = alt_name;
    jclass["ALIAS"] = sym_class->name;
    jclass["BASE_CLASSES"] = nlohmann::json::object();
    jclass["SERIALIZE_JSON_FN"] = nullptr;
    jclass["DESERIALIZE_JSON_FN"] = nullptr;

    for (auto& base : sym_class->base_classes) {
        std::string base_alt_name = base->global_qualified_name;
        std::replace(base_alt_name.begin(), base_alt_name.end(), ':', '_');
        jclass["BASE_CLASSES"][base_alt_name]["DECL_NAME"] = base->global_qualified_name;
    }

    // TODO:
    for (auto& kv : sym_class->nested_symbol_table->symbols) {
        auto& vec = kv.second;
        for (int i = 0; i < vec.size(); ++i) {
            auto& sym = vec[i];
            switch (sym->get_type_enum()) {
            case e_symbol_object:
                make_reflection_json_member_object(
                    dynamic_cast<symbol_object*>(sym.get()),
                    jobjects
                );
                break;
            case e_symbol_function:
                make_reflection_json_member_function(
                    sym, sym_class_ref,
                    jclass, jfunctions, jprops
                );
                break;
            case e_symbol_class:
                if (sym->no_reflect) {
                    printf("template json: no_reflect: %s\n", sym->global_qualified_name.c_str());
                    continue;
                }
                if (!sym->defined) {
                    printf("template json warning: symbol %s not defined, will not appear in template json\n", sym->global_qualified_name.c_str());
                    continue;
                }
                make_reflection_template_data(sym, json);
                break;
            case e_symbol_enum:
                if (sym->no_reflect) {
                    printf("template json: no_reflect: %s\n", sym->global_qualified_name.c_str());
                    continue;
                }
                if (!sym->defined) {
                    printf("template json warning: symbol %s not defined, will not appear in template json\n", sym->global_qualified_name.c_str());
                    continue;
                }
                make_reflection_template_data(sym, json);
                break;
            }
        }
    }
    /*
    for (auto& kv2 : sym->nested_symbol_table->objects) {
        auto object_sym = (symbol_object*)kv2.second.get();
        auto& sym = kv2.second;
        printf("'%s': %s\n", sym->file->filename_canonical.c_str(), sym->global_qualified_name.c_str());

        auto& jobject = jobjects.emplace_back();// [kv2.second->global_qualified_name];
        jobject["DECL_NAME"] = sym->global_qualified_name;
        jobject["ALIAS"] = sym->name;
        jobject["NAME"] = sym->name;
        jobject["DECL_TYPE"] = object_sym->type_id_.make_string();
    }*/
    /*
    for (auto& kv2 : sym->nested_symbol_table->functions) {
        auto& sym_func = kv2.second;
        printf("'%s': %s\n", sym_func->file->filename_canonical.c_str(), sym_func->global_qualified_name.c_str());

        symbol_function* func = (symbol_function*)sym_func.get();
        for (auto& overload : func->overloads) {
            attribute* attrib_get = overload->attrib_spec.find_attrib("get");
            attribute* attrib_set = overload->attrib_spec.find_attrib("set");
            attribute* attrib_serialize_json = overload->attrib_spec.find_attrib("serialize_json");
            attribute* attrib_deserialize_json = overload->attrib_spec.find_attrib("deserialize_json");
            if (attrib_get) {
                assert(attrib_get->value.is_string());
                std::string prop_name = attrib_get->value.get_string();
                jprops[prop_name]["get"] = function_overload_to_json(overload.get(), sym);

                type_id tid = overload->type_id_.get_return_type();
                jprops[prop_name]["type"] = tid.make_string(true, true, true);
            }
            if (attrib_set) {
                assert(attrib_set->value.is_string());
                std::string prop_name = attrib_set->value.get_string();
                jprops[prop_name]["set"] = function_overload_to_json(overload.get(), sym);

                type_id tid = overload->type_id_.get_return_type();
                jprops[prop_name]["type"] = tid.make_string(true, true, true);
            }
            if (attrib_serialize_json) {
                jclass["SERIALIZE_JSON_FN"] = func->name;
            }
            if (attrib_deserialize_json) {
                jclass["DESERIALIZE_JSON_FN"] = func->name;
            }
        }
    }*/
    std::vector<std::string> props_to_erase;
    for (auto& it = jprops.begin(); it != jprops.end(); ++it) {
        int set = it.value().count("set");
        int get = it.value().count("get");
        if (set && !get) {
            printf("template json warning: property %s has a setter, but not a getter, not supported, removing\n", it.key().c_str());
            props_to_erase.push_back(it.key());
            continue;
        }
        if (set == 0) {
            it.value()["set"] = nullptr;
        }
        if (get == 0) {
            it.value()["get"] = nullptr;
        }
    }
    for (auto& key : props_to_erase) {
        jprops.erase(key);
    }
    /*
    for (auto& kv : sym->nested_symbol_table->types) {
        auto& sym = kv.second;
        if (sym->no_reflect) {
            printf("template json: no_reflect: %s\n", sym->global_qualified_name.c_str());
            continue;
        }
        if (!sym->defined) {
            printf("template json warning: symbol %s not defined, will not appear in template json\n", sym->global_qualified_name.c_str());
            continue;
        }
        make_reflection_template_data(sym, json);
    }*/
    /*
    for (auto& kv : sym->nested_symbol_table->enums) {
        auto& sym = kv.second;
        if (sym->no_reflect) {
            printf("template json: no_reflect: %s\n", sym->global_qualified_name.c_str());
            continue;
        }
        if (!sym->defined) {
            printf("template json warning: symbol %s not defined, will not appear in template json\n", sym->global_qualified_name.c_str());
            continue;
        }
        make_reflection_template_data(sym, json);
    }*/
}
void make_reflection_json_enum(symbol_enum* enum_sym, nlohmann::json& json) {
    printf("ENUM %s\n", enum_sym->global_qualified_name.c_str());
    std::string alt_name = enum_sym->global_qualified_name;
    std::replace(alt_name.begin(), alt_name.end(), ':', '_');
    std::string enum_key = enum_key_to_string(enum_sym->key);

    json["HEADER_NAME"] = enum_sym->file->file_name;
    auto& jenums = json["ENUMS"];
    auto& jenum = jenums[alt_name];
    auto& jenumerators = jenum["ENUMERATORS"];
    jenum["DECL_NAME"] = enum_sym->global_qualified_name;
    jenum["ALT_NAME"] = alt_name;
    jenum["ALIAS"] = enum_sym->name;
    jenum["FORWARD_DECL"] = enum_key + " " + enum_sym->global_qualified_name;
    jenumerators = nlohmann::json::object();

    for (auto& kv : enum_sym->nested_symbol_table->symbols) {
        const auto& vec = kv.second;
        for (int i = 0; i < vec.size(); ++i) {
            const auto& sym = vec[i];
            if (sym->get_type_enum() != e_symbol_enumerator) {
                continue;
            }

            printf("'%s': %s\n", sym->file->filename_canonical.c_str(), sym->global_qualified_name.c_str());
            jenumerators[sym->global_qualified_name] = 0;
        }
    }
    /*
    for (auto& kv : enum_sym->nested_symbol_table->enumerators) {
        auto& sym = kv.second;
        printf("'%s': %s\n", sym->file->filename_canonical.c_str(), sym->global_qualified_name.c_str());
        jenumerators[sym->global_qualified_name] = 0;
    }*/
}

// NOTE: here template means inja template
void make_reflection_template_data(std::shared_ptr<symbol>& sym, nlohmann::json& json) {
    if (sym->is<symbol_class>()) {
        make_reflection_json_class(sym, json);
    } else if(sym->is<symbol_enum>()) {
        make_reflection_json_enum(dynamic_cast<symbol_enum*>(sym.get()), json);
    }
}


void gather_all_symbols(symbol_table* table, std::vector<symbol_ref>& all_symbols) {
    uint32_t mask
        = LOOKUP_FLAG_CLASS
        | LOOKUP_FLAG_ENUM
        | LOOKUP_FLAG_OBJECT
        | LOOKUP_FLAG_FUNCTION
        | LOOKUP_FLAG_NAMESPACE;
    
    for (auto& kv : table->symbols) {
        auto& vec = kv.second;
        if ((vec.mask & mask) == 0) {
            continue;
        }
        for (int i = 0; i < vec.size(); ++i) {
            auto& sym = vec[i];
            if (!sym->is_defined()) {
                continue;
            }
            if (sym->no_reflect) {
                continue;
            }
            if (((0x1 << sym->get_type_enum()) & mask) == 0) {
                continue;
            }

            if (sym->get_type_enum() == e_symbol_namespace) {
                gather_all_symbols(sym->nested_symbol_table.get(), all_symbols);
            } else {
                all_symbols.push_back(sym);
            }
        }
    }
    /*
    for (auto& kv : table->types) {
        auto& sym = kv.second;
        all_symbols.push_back(sym);
    }
    for (auto& kv : table->enums) {
        auto& sym = kv.second;
        all_symbols.push_back(sym);
    }
    for (auto& kv : table->objects) {
        auto& sym = kv.second;
        all_symbols.push_back(sym);
    }
    for (auto& kv : table->functions) {
        auto& sym = kv.second;
        all_symbols.push_back(sym);
    }
    

    for (auto& kv : table->namespaces) {
        gather_all_symbols(kv.second->nested_symbol_table.get(), all_symbols);
    }*/
}

static std::string per_file_header_template;
static std::string header_template;
static std::string src_template;
void make_reflection_files(const std::string& output_dir, std::vector<translation_unit>& units, const std::string& unity_filename_override) {
    struct file_symbols {
        std::unordered_map<std::string, symbol_ref> name_to_sym;
    };
    std::unordered_map<std::string, std::unique_ptr<file_symbols>> symbols_per_file;
    std::unordered_map<std::string, nlohmann::json> tpl_data_map;

    for (int i = 0; i < units.size(); ++i) {
        auto& unit = units[i];
        auto& ps = unit.ps;
        auto& scope = ps->get_root_scope();

        std::vector<symbol_ref> all_symbols;
        gather_all_symbols(scope.get(), all_symbols);

        for (auto& sym : all_symbols) {
            if (!sym->file) {
                printf("%s symbol skipped due to unknown file of origin\n", sym->global_qualified_name.c_str());
                continue;
            }
            std::experimental::filesystem::path path = sym->file->file_name;
            if (path.extension() == ".cpp" || path.extension() == ".c" || path.extension() == ".cxx") {
                printf(
                    "warning: '%s': Symbol %s is declared in a c/cpp/cxx file, ignoring. Please only mark symbols for reflection in header files\n",
                    sym->file->file_name.c_str(),
                    sym->global_qualified_name.c_str()
                );
                continue;
            }

            auto& file_syms = symbols_per_file[sym->file->filename_canonical];
            if (!file_syms) {
                file_syms.reset(new file_symbols);
            }
            auto it = file_syms->name_to_sym.find(sym->global_qualified_name);
            if (it != file_syms->name_to_sym.end()) {
                continue;
            }
            file_syms->name_to_sym.insert(
                std::make_pair(sym->global_qualified_name, sym)
            );
        }
    }

    for (auto& kv : symbols_per_file) {
        auto& fname = kv.first;
        auto& symbols = kv.second->name_to_sym;
        if (tpl_data_map.count(fname) == 0) {
            auto& json = tpl_data_map[fname];
            json["CLASSES"] = nlohmann::json::object();
            json["ENUMS"] = nlohmann::json::object();
        }
        auto& json = tpl_data_map[fname];
        for (auto& kv2 : symbols) {
            auto& sym = kv2.second;
            if (sym->no_reflect) {
                printf("no_reflect: %s\n", sym->global_qualified_name.c_str());
                continue;
            }
            if (!sym->defined) {
                printf("Symbol %s not defined, will not appear in template json\n", sym->global_qualified_name.c_str());
                continue;
            }
            make_reflection_template_data(sym, json);
        }
    }

    if(!per_file_header_template.empty()) {
        for (auto& kv : tpl_data_map) {
            auto& fname = kv.first;
            auto& json = kv.second;

            printf("%s\n", json.dump(4).c_str());

            inja::Environment env;

            std::string result;
            try {
                result = env.render(per_file_header_template.c_str(), json);
                //printf("%s\n", result.c_str());
            } catch(inja::RenderError& ex) {
                printf("template render error: %s\n", ex.what());
                continue;
            } catch(inja::ParserError& ex) {
                printf("template parser error: %s\n", ex.what());
                continue;
            } catch(inja::JsonError& ex) {
                printf("template json error: %s\n", ex.what());
                continue;
            } catch(std::exception& ex) {
                printf("generic exception: %s\n", ex.what());
                continue;
            }

            std::experimental::filesystem::path path = fname;
            path.replace_extension(".auto" + path.extension().string());

            {
                FILE* f = fopen(path.string().c_str(), "wb");
                if (!f) {
                    printf("error: failed to create file '%s'\n", path.string().c_str());
                    return;
                }
                size_t written = fwrite(result.data(), result.size(), 1, f);
                if (written != 1) {
                    fclose(f);
                    printf("error: failed to write to file '%s'\n", path.string().c_str());
                    return;
                }
                fclose(f);
                printf("output: %s\n", path.string().c_str());
            }
        }
    }

    std::string unity_filename = "cppi_reflect";
    if (!unity_filename_override.empty()) {
        unity_filename = unity_filename_override;
    }

    std::experimental::filesystem::path unity_cpp_path = output_dir + "/" + unity_filename + ".auto.cpp";
    unity_cpp_path = std::experimental::filesystem::canonical(unity_cpp_path);
    std::experimental::filesystem::path header_path = output_dir + "/" + unity_filename + ".auto.hpp";
    header_path = std::experimental::filesystem::canonical(header_path);
    
    std::experimental::filesystem::path unity_cpp_dir = unity_cpp_path;
    unity_cpp_dir.remove_filename();
    printf("cpp canonical   : %s\n", unity_cpp_path.string().c_str());
    printf("cpp dir         : %s\n", unity_cpp_dir.string().c_str());

    fsCreateDirRecursive(unity_cpp_dir.string());

    nlohmann::json json_unity = nlohmann::json::object();
    json_unity["INCLUDE_FILES"] = nlohmann::json::array();
    json_unity["CLASSES"] = nlohmann::json::object();
    json_unity["ENUMS"] = nlohmann::json::object();
    for (auto& kv : tpl_data_map) {
        //printf("%s\n", kv.second.dump(4).c_str());
        /*
        auto& jclasses = json_unity["CLASSES"];
        if (kv.second.count("CLASSES")) {
            jclasses.merge_patch(kv.second["CLASSES"]);
        }*/
        json_unity.merge_patch(kv.second);
        
        std::string relative_incl_path = fsMakeRelativePath(unity_cpp_dir.string(), kv.first);
        std::replace(relative_incl_path.begin(), relative_incl_path.end(), '\\', '/');
        json_unity["INCLUDE_FILES"].push_back(relative_incl_path);
    }
    //printf("%s\n", json_unity.dump(4).c_str());

    if(!header_template.empty()) {
        inja::Environment env;
        std::string result;
        try {
            result = env.render(header_template.c_str(), json_unity);
            //printf("%s\n", result.c_str());
        } catch(inja::RenderError& ex) {
            printf("template render error: %s\n", ex.what());
            return;
        } catch(inja::ParserError& ex) {
            printf("template parser error: %s\n", ex.what());
            return;
        } catch(inja::JsonError& ex) {
            printf("template json error: %s\n", ex.what());
            return;
        } catch(std::exception& ex) {
            printf("generic exception: %s\n", ex.what());
            return;
        }
        
        {
            FILE* f = fopen(header_path.string().c_str(), "wb");
            if (!f) {
                printf("error: failed to create file '%s'\n", header_path.string().c_str());
                return;
            }
            size_t written = fwrite(result.data(), result.size(), 1, f);
            if (written != 1) {
                fclose(f);
                printf("error: failed to write to file '%s'\n", header_path.string().c_str());
                return;
            }
            fclose(f);
            printf("output: %s\n", header_path.string().c_str());
        }
    }

    if(!src_template.empty()) {
        inja::Environment env;
        std::string result;
        try {
            result = env.render(src_template.c_str(), json_unity);
            //printf("%s\n", result.c_str());
        } catch(inja::RenderError& ex) {
            printf("template render error: %s\n", ex.what());
            return;
        } catch(inja::ParserError& ex) {
            printf("template parser error: %s\n", ex.what());
            return;
        } catch(inja::JsonError& ex) {
            printf("template json error: %s\n", ex.what());
            return;
        } catch(std::exception& ex) {
            printf("generic exception: %s\n", ex.what());
            return;
        }
        
        {
            FILE* f = fopen(unity_cpp_path.string().c_str(), "wb");
            if (!f) {
                printf("error: failed to create file '%s'\n", unity_cpp_path.string().c_str());
                return;
            }
            size_t written = fwrite(result.data(), result.size(), 1, f);
            if (written != 1) {
                fclose(f);
                printf("error: failed to write to file '%s'\n", unity_cpp_path.string().c_str());
                return;
            }
            fclose(f);
            printf("output: %s\n", unity_cpp_path.string().c_str());
        }
    }

#ifdef _WIN32
    //SetConsoleOutputCP(65001);
#endif
    //printf("( \u00DB e \u00DB )\n");
    //printf("\u0028\u0020\u0361\u00B0\u0020\u035C\u0296\u0020\u0361\u00B0\u0029\u2501\u2606\uFF9F\u002E\u002A\uFF65\uFF61\uFF9F\n");
}

int main(int argc, char* argv[]) {
    printf("CPPI tool v0.1\n");

    HANDLE hConsole = INVALID_HANDLE_VALUE;
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

    // Command line
    bool force_full_parse = false;
    bool dump_pp = false;
    bool ignore_missing_includes = false;
    std::string output_dir;
    std::string per_file_header_template_filename;
    std::string header_template_filename;
    std::string src_template_filename;
    std::string unity_filename_override;
    std::string fwd_decl_filename;
    std::vector<std::string> include_dirs;
    std::vector<std::string> source_files;
    {
        popl::OptionParser op("Allowed commands");
        auto help_op = op.add<popl::Switch>("h", "help", "produce help message");
        auto include_op = op.add<popl::Value<std::string>>("I", "include", "additional inclue directory");
        auto dump_preprocessed_op = op.add<popl::Switch>("P", "ppdump", "dump preprocessed code into a file");
        auto force_full_op = op.add<popl::Switch>("F", "force", "force full parse even if cache is fresh");
        auto ignore_missing_includes_op = op.add<popl::Switch>("g", "ignore", "ignore failed include directives");
        auto output_op = op.add<popl::Value<std::string>>("O", "output", "output directory for unity cpp and header files");
        auto per_file_header_tpl_op = op.add<popl::Value<std::string>>("t", "header_template", "an inja template file for individual headers");
        auto cpp_tpl_op = op.add<popl::Value<std::string>>("U", "unity_template", "an inja template file for unity cpp file");
        auto header_tpl_op = op.add<popl::Value<std::string>>("u", "unity_header_template", "an inja template file for unity cpp file header");
        auto unity_file_verride_op = op.add<popl::Value<std::string>>("N", "project_name", "overrides the stem part of a unity file filename as well as its header");
        auto fwd_decl_op = op.add<popl::Value<std::string>>("f", "decl", "path to a file containing forward declarations, included in every translation unit");
        op.parse(argc, argv);
        
        if (help_op->is_set()) {
            printf("%s\n", op.help().c_str());
        }

        if (include_op->count()) {
            printf("Include directories supplied: \n");
            include_dirs.reserve(include_op->count());
            for (int i = 0; i < include_op->count(); ++i) {
                include_dirs.push_back(include_op->value(i));
                printf("\t%s\n", include_op->value(i).c_str());
            }
        }

        if (dump_preprocessed_op->is_set()) {
            dump_pp = true;
        }

        if (force_full_op->is_set()) {
            force_full_parse = true;
        }

        if (ignore_missing_includes_op->is_set()) {
            ignore_missing_includes = true;
        }

        if (output_op->count()) {
            output_dir = output_op->value(0);
        }
        if (per_file_header_tpl_op->count()) {
            per_file_header_template_filename = per_file_header_tpl_op->value(0);
        }
        if (header_tpl_op->count()) {
            header_template_filename = header_tpl_op->value(0);
        }
        if (cpp_tpl_op->count()) {
            src_template_filename = cpp_tpl_op->value(0);
        }
        if (unity_file_verride_op->count()) {
            unity_filename_override = unity_file_verride_op->value(0);
        }
        if (fwd_decl_op->count()) {
            fwd_decl_filename = fwd_decl_op->value(0);
        }

        source_files.reserve(op.non_option_args().size());
        for (int i = 0; i < op.non_option_args().size(); ++i) {
            const std::string& spath = op.non_option_args()[i];
            std::experimental::filesystem::path path = spath;
            std::experimental::filesystem::path check_path = path;
            check_path.replace_extension("");
            if (check_path.extension() == ".auto") {
                printf("Input file '%s' has .auto in its name, ignoring\n", path.string().c_str());
                continue;
            }
            printf("%s\n", check_path.string().c_str());
            path = std::experimental::filesystem::canonical(path);
            source_files.push_back(path.string());
        }
    }

    // Setup attribute interpretations
    set_attribute_type("get", ATTRIB_STRING_LITERAL);
    set_attribute_type("set", ATTRIB_STRING_LITERAL);

    // Load templates
    {
        if (!per_file_header_template_filename.empty()) {
            if (!fsSlurpFile(per_file_header_template_filename, per_file_header_template)) {
                printf("Failed to open per-file-header-template file");
                return -1;
            }
        }
        if (!header_template_filename.empty()) {
            if (!fsSlurpFile(header_template_filename, header_template)) {
                printf("Failed to open header template file");
                return -1;
            }
        }
        if (!src_template_filename.empty()) {
            if (!fsSlurpFile(src_template_filename, src_template)) {
                printf("Failed to open unity source file template");
                return -1;
            }
        }
    }

    if (source_files.empty()) {
        printf("No source files supplied\n");
        return -1;
    }
    if (output_dir.empty()) {
        printf("No output filename supplied\n");
        return -1;
    }
    std::unordered_set<std::string> source_file_set;
    for (int i = 0; i < source_files.size(); ++i) {
        source_file_set.insert(source_files[i]);
    }

    std::string new_cmd_cache = make_cmd_cache(argc, argv);

    bool full_parse = false;

    cache_data cache;
    if (read_cache_data(cache)) {
        if (new_cmd_cache != cache.cmd_cache) {
            cache.cmd_cache = new_cmd_cache;
            cache.source_files = source_files;
            full_parse = true;
            printf("Performing full parse due to changed command line\n");
        } else if(force_full_parse) {
            full_parse = true;
            cache.source_files = source_files;
            printf("Performing full parse due to the -F command\n");
        }
    } else {
        cache.cmd_cache = new_cmd_cache;
        cache.source_files = source_files;
        full_parse = true;
        printf("Performing full parse due to absent cache file\n");
    }

    if (!full_parse) {
        std::unordered_set<std::string> changed_files;
        for (auto& it : cache.timestamps) {
            auto& fname = it.first;
            time_t timestamp = it.second;
            std::chrono::system_clock::time_point tp
                = std::experimental::filesystem::last_write_time(fname);
            time_t new_timestamp = std::chrono::system_clock::to_time_t(tp);
            if (new_timestamp != timestamp) {
                changed_files.insert(fname);
            }
        }

        std::unordered_set<std::string> filtered_sources;
        if (!changed_files.empty()) {
            printf("Changes since last parse: \n");
            for (auto& src : changed_files) {
                printf("\t%s\n", src.c_str());
                find_dependent_sources(cache, source_file_set, src, filtered_sources);
            }
        }
        if (filtered_sources.size() > 0) {
            printf("Source files need parsing due to changes: \n");
            for (auto& src : filtered_sources) {
                printf("\t%s\n", src.c_str());
            }
        }

        source_files.clear();
        source_files.reserve(filtered_sources.size());
        for (auto& src : filtered_sources) {
            source_files.push_back(src);
        }
    }

    std::vector<translation_unit> translation_units;
    translation_units.resize(source_files.size());

    if (!source_files.empty()) {
        for (int i = 0; i < source_files.size(); ++i) {
            auto& tu = translation_units[i];
            std::string& fname = source_files[i];
            printf("parsing: %s\n", fname.c_str());

            tu.pp.reset(new preprocessor(ignore_missing_includes));
            tu.pp->add_include_directories(include_dirs);
            tu.pp->predefine_macro("_M_X64", "100");
            tu.pp->predefine_macro("_M_AMD64", "100");
            tu.pp->predefine_macro("_WIN32", "1");
            tu.pp->predefine_macro("_WIN64", "1");
            tu.pp->predefine_macro("CPPI_PARSER", "1");

            if (!tu.pp->init(fname.c_str(), true, false)) {
                printf("Failed to open source file \"%s\"\n", fname.c_str());
                return -1;
            }

            try {
                if (!fwd_decl_filename.empty()) {
                    printf("including forward declarations from '%s'\n", fwd_decl_filename.c_str());
                    if (!tu.pp->include_file_current_dir(fwd_decl_filename)) {
                        throw pp_exception("Failed to open forward declarations file '%s'", 0, 0, fwd_decl_filename.c_str());
                    }
                }

                tu.ps.reset(new parse_state(tu.pp.get(), PARSE_LIMITED));

                eat_translation_unit_2(*tu.ps.get());
            }
            catch (const parse_exception& ex) {
                dbg_printf_color("%s\n", DBG_ERROR, ex.what());
                return -1;
            }
            catch (const pp_exception& ex) {
                dbg_printf_color("%s\n", DBG_ERROR, ex.what());
                return -1;
            }
            catch (const pp_exception_notimplemented& ex) {
                dbg_printf_color("%s\n", DBG_NOT_IMPLEMENTED, ex.what());
                return -1;
            }
            catch (const std::exception& ex) {
                dbg_printf_color("generic exception: %s\n", DBG_ERROR, ex.what());
                return -1;
            }
            if (dump_pp) {
                FILE* f = fopen((fname + ".pp").c_str(), "wb");
                fwrite(tu.pp->get_preprocessed_text().c_str(), tu.pp->get_preprocessed_text().size(), 1, f);
                fclose(f);
            }
        }

        for (auto& it : pp_get_file_cache()) {
            cache.timestamps[it.first] = it.second->last_write_time;
            auto& dependants = cache.dependant_sets[it.first];
            dependants.clear();
            for (auto& dep_other : it.second->dependants) {
                dependants.push_back(dep_other->filename_canonical);
            }
        }

        make_reflection_files(output_dir, translation_units, unity_filename_override);

        write_cache_data(cache);
    } else {
        printf("Everything is up to date\n");
    }

    return 0;
}
