#pragma once

#include "parse_state.hpp"
#include "parse_exception.hpp"


class attribute {
    std::unordered_map<std::string, std::unique_ptr<attribute>> attribs;
    std::vector<token> tokens;
public:
    std::string name;

    attribute() {}
    attribute(const attribute& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
            );
        }
        tokens = other.tokens;
    }
    attribute(attribute&& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::move(it.second))
            );
        }
        tokens = std::move(other.tokens);
    }

    void merge(const attribute& source) {
        for (auto& it : source.attribs) {
            auto& it2 = attribs.find(it.first);
            if (it2 == attribs.end()) {
                attribs.insert(
                    std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
                );
                continue;
            }
            it2->second->merge(*it.second.get());
        }
        tokens.insert(tokens.end(), source.tokens.begin(), source.tokens.end());
    }

    int token_count() const {
        return tokens.size();
    }
    int attrib_count() const {
        return attribs.size();
    }
    attribute* getadd_attrib(const std::string& name) {
        attribute* ptr = find_attrib(name);
        if (ptr) {
            return ptr;
        }
        ptr = new attribute();
        ptr->name = name;
        attribs.insert(std::make_pair(name, std::unique_ptr<attribute>(ptr)));
        return ptr;
    }
    void add_token(const token& tok) {
        tokens.push_back(tok);
    }
    void add_tokens(const token* in_tokens, int count) {
        tokens.insert(tokens.end(), in_tokens, in_tokens + count);
    }

    attribute* find_attrib(const std::string& name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }
    const token* get_tokens() {
        return &tokens[0];
    }
    const token& get_token(int i) const {
        return tokens[i];
    }
};
class attribute_specifier {
    std::unordered_map<std::string, std::unique_ptr<attribute>> attribs;
public:
    attribute_specifier() {}
    attribute_specifier(const attribute_specifier& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
            );
        }
    }
    attribute_specifier(attribute_specifier&& other) {
        attribs.clear();
        for (auto& it : attribs) {
            attribs.insert(
                std::make_pair(it.first, std::move(it.second))
            );
        }
    }

    void merge(const attribute_specifier& source) {
        for (auto& it : source.attribs) {
            auto& it2 = attribs.find(it.first);
            if (it2 == attribs.end()) {
                attribs.insert(
                    std::make_pair(it.first, std::unique_ptr<attribute>(new attribute(*it.second.get())))
                );
                continue;
            }
            it2->second->merge(*it.second.get());
        }
    }

    int attrib_count() const {
        return attribs.size();
    }
    attribute* getadd_attrib(const std::string& name) {
        attribute* ptr = find_attrib(name);
        if (ptr) {
            return ptr;
        }
        ptr = new attribute();
        ptr->name = name;
        attribs.insert(std::make_pair(name, std::unique_ptr<attribute>(ptr)));
        return ptr;
    }

    attribute* find_attrib(const std::string& name) {
        auto it = attribs.find(name);
        if (it == attribs.end()) {
            return 0;
        }
        return it->second.get();
    }
};


bool eat_attribute_specifier(parse_state& ps, attribute_specifier& attr_spec = attribute_specifier());
bool eat_attribute_specifier_seq(parse_state& ps, attribute_specifier& attr_spec = attribute_specifier());