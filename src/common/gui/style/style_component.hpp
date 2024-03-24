#pragma once

#include <unordered_map>
#include <exception>
#include "gui/types.hpp"

using GUI_STYLE_FLAGS = uint32_t;
constexpr uint32_t GUI_STYLE_FLAG_HOVERED   = 0x0001;
constexpr uint32_t GUI_STYLE_FLAG_PRESSED   = 0x0002;
constexpr uint32_t GUI_STYLE_FLAG_FOCUSED   = 0x0004;
constexpr uint32_t GUI_STYLE_FLAG_ACTIVE    = 0x0008;
constexpr uint32_t GUI_STYLE_FLAG_DISABLED  = 0x0010;
constexpr uint32_t GUI_STYLE_FLAG_SELECTED  = 0x0020;
constexpr uint32_t GUI_STYLE_FLAG_READONLY  = 0x0040;
inline const char* guiStyleFlagToString(uint32_t flag) {
    switch (flag) {
    case GUI_STYLE_FLAG_HOVERED:
        return "hovered";
    case GUI_STYLE_FLAG_PRESSED:
        return "pressed";
    case GUI_STYLE_FLAG_FOCUSED:
        return "focused";
    case GUI_STYLE_FLAG_ACTIVE:
        return "active";
    case GUI_STYLE_FLAG_DISABLED:
        return "disabled";
    case GUI_STYLE_FLAG_SELECTED:
        return "selected";
    case GUI_STYLE_FLAG_READONLY:
        return "readonly";
    default:
        return "unknown";
    }
}


namespace gui {
    
    struct nullopt_t {
        struct init {};
        explicit constexpr nullopt_t(init) {}
    };

    struct inherit_t {
        struct init {};
        explicit constexpr inherit_t(init) {}
    };

    constexpr nullopt_t nullopt{ nullopt_t::init() };
    constexpr inherit_t inherit{ inherit_t::init() };

    template<typename T>
    class style_value {
        T value_;
        enum class state_t {
            empty,
            inherit,
            value
        };
        state_t state = state_t::empty;
    public:
        style_value() : value_(T()), state(state_t::empty) {}
        style_value(const T& value) : value_(value), state(state_t::value) {}
        style_value(nullopt_t) : state(state_t::empty) {}
        style_value(inherit_t) : state(state_t::inherit) {}
        
        void reset() { state = state_t::empty; }

        void merge(const style_value<T>& other, bool empty_inherits) {
            if (other.is_inherit()) {
                return;
            }
            if (other.has_value()) {
                value_ = other.value_;
                state = state_t::value;
                return;
            }
            if (empty_inherits) {
                return;
            }
            value_ = T();
            state = state_t::empty;
        }
        void inherit(const style_value<T>& other) {
            if (!this->is_inherit()) {
                return;
            }
            if (other.is_inherit() || other.is_empty()) {
                return;
            }
            value_ = other.value_;
            state = state_t::value;
        }

        bool has_value() const { return state == state_t::value; }
        bool is_inherit() const { return state == state_t::inherit; }
        bool is_empty() const { return state == state_t::empty; }

        T& value(const T& default_ = T()) {
            if (!has_value()) {
                // Do not set state as state_t::value;
                value_ = default_;
            }
            return value_;
        }
        const T& value(const T& default_ = T()) const {
            if (!has_value()) {
                // Do not set state as state_t::value;
                value_ = default_;
            }
            return value_;
        }
    };

    class style_component {
    public:
        virtual ~style_component() {}
        virtual style_component* clone_new() const = 0;
        virtual void merge(const style_component& other, bool empty_inherits) = 0;
        virtual void _inherit(const style_component& other) = 0;
        virtual void finalize() {}
        virtual bool always_inherit() = 0;
    };
    template<typename T, bool ALWAYS_INHERIT>
    class style_component_t : public style_component {
    public:
        virtual style_component* clone_new() const { return new T(static_cast<const T&>(*this)); };
        void merge(const style_component& other, bool empty_inherits) override {
            on_merge(static_cast<const T&>(other), empty_inherits);
        }
        void _inherit(const style_component& other) override {
            on_inherit(static_cast<const T&>(other));
        }
        virtual void on_merge(const T& other, bool empty_inherits) = 0;
        virtual void on_inherit(const T& other) = 0;
        bool always_inherit() override { return ALWAYS_INHERIT; }
    };

    class style;
    struct style_prop {
        virtual ~style_prop() {}
        virtual void apply(style& s) const = 0;
        virtual style_prop* clone_new() const = 0;
    };

    struct style_prop_proxy {
        ::std::unique_ptr<style_prop> prop;
        style_prop_proxy(const style_prop& prop)
            : prop(prop.clone_new()) {}
    };


    class style {
        std::unordered_map<type, std::unique_ptr<gui::style_component>> components;
    public:
        style() {}
        style(const std::initializer_list<style_prop_proxy>& list) {
            for (auto& p : list) {
                p.prop->apply(*this);
            }
        }

        void clear() {
            components.clear();
        }

        void inherit(const style& other) {
            for (auto& other_kv : other.components) {
                auto it = components.find(other_kv.first);
                if (it == components.end()) {
                    if (other_kv.second->always_inherit()) {
                        components[other_kv.first].reset(other_kv.second->clone_new());
                    }
                    continue;
                }
                it->second->_inherit(*other_kv.second.get());
            }
        }
        void merge(const style& other, bool empty_inherits) {
            for (auto& other_kv : other.components) {
                auto it = components.find(other_kv.first);
                if (it == components.end()) {
                    components[other_kv.first].reset(other_kv.second->clone_new());
                    continue;
                }
                it->second->merge(*other_kv.second.get(), empty_inherits);
            }
        }
        
        style& operator+(const style_prop& prop) {
            prop.apply(*this);
            return *this;
        }

        void finalize() {
            for (auto& kv : components) {
                kv.second->finalize();
            }
        }

        template<typename T>
        bool has_component() { return components.count(type_get<T>()); }
        template<typename T>
        T* get_component() {
            auto it = components.find(type_get<T>());
            if (it == components.end()) {
                return 0;
            }
            return (T*)it->second.get();
        }
        template<typename T>
        style* add_component(const T& style_) {
            auto ptr = new T(style_);
            components.insert(std::make_pair(type_get<T>(), std::unique_ptr<gui::style_component>(ptr)));
            return this;
        }
        template<typename T>
        T* add_component() {
            auto ptr = new T;
            components.insert(std::make_pair(type_get<T>(), std::unique_ptr<gui::style_component>(ptr)));
            return ptr;
        }
    };


    class style_sheet {
        std::unordered_map<std::string, std::unique_ptr<style>> styles;
    public:
        style_sheet& add(const char* selector, const std::initializer_list<style_prop_proxy>& list) {
            styles[selector].reset(new style(list));
            return *this;
        }

        std::list<style*> select_styles(style* out_style, const std::list<std::string>& classes, GUI_STYLE_FLAGS flags = 0) {
            if (classes.empty()) {
                return std::list<style*>();
            }

            static const uint32_t flag_array[] = {
                GUI_STYLE_FLAG_HOVERED,
                GUI_STYLE_FLAG_PRESSED,
                GUI_STYLE_FLAG_FOCUSED,
                GUI_STYLE_FLAG_ACTIVE,
                GUI_STYLE_FLAG_DISABLED,
                GUI_STYLE_FLAG_SELECTED,
                GUI_STYLE_FLAG_READONLY
            };
            static const int flag_count = sizeof(flag_array) / sizeof(uint32_t);

            std::list<style*> selected;
            for (auto& class_sel : classes) {
                auto it = styles.find(class_sel);
                if (it != styles.end()) {
                    out_style->merge(*it->second.get(), false);
                    selected.push_back(it->second.get());
                }

                for (int i = 0; i < flag_count; ++i) {
                    auto flag = flag_array[i];
                    if ((flags & flag) == 0) {
                        continue;
                    }
                    std::string cls = class_sel + std::string(":") + guiStyleFlagToString(flag);
                    auto it = styles.find(cls);
                    if (it != styles.end()) {
                        out_style->merge(*it->second.get(), true);
                        selected.push_back(it->second.get());
                    }
                }
            }

            return selected;
        }
    };

}