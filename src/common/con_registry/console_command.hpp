#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <vector>


class ConsoleCommand {
    std::string source;
    std::string_view name_;
    std::vector<std::string_view> args;

    template<typename T>
    static std::optional<T> parse_arg(std::string_view s) {
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>) {
            return T(s);
        } else if constexpr (std::is_integral_v<T>) {
            T val{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
            if(ec != std::errc{}) return std::nullopt;
            return val;
        } else if constexpr (std::is_floating_point_v<T>) {
            T val{};
            auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), val);
            if(ec != std::errc{}) return std::nullopt;
            return val;
        } else {
            static_assert(sizeof(T) == 0, "no parse_arg specialization for this type");
        }
    }
public:
    ConsoleCommand(const std::string& str)
    : source(str) {
        const char* p = source.data();
        const char* end = p + source.size();

        auto skip_whitespace = [&] {
            while(p < end && isspace((unsigned char)*p)) ++p;
        };
        auto read_token = [&]()->std::string_view {
            if (p < end && *p == '"') {
                ++p;
                const char* start = p;
                while(p < end && *p != '"') ++p;
                std::string_view tok(start, p - start);
                if(p < end) ++p;
                return tok;
            }
            const char* start = p;
            while(p < end && !isspace((unsigned char)*p)) ++p;
            return { start, (size_t)(p - start) };
        };

        skip_whitespace();
        if(p < end) name_ = read_token();
        while (true) {
            skip_whitespace();
            if(p >= end) break;
            args.push_back(read_token());
        }
    }

    std::string_view name() const {
        std::string n(name_);
        for (int i = 0; i < n.size(); ++i) {
            n[i] = std::tolower(n[i]);
        }
        return name_;
    }

    size_t arg_count() const { return args.size(); }

    template<typename T>
    std::optional<T> try_arg(int idx) const {
        if(idx < 0 || idx >= args.size()) return std::nullopt;
        return parse_arg<T>(args[idx]);
    }

    template<typename T>
    T arg(int idx, T default_val = {}) const {
        return try_arg<T>(idx).value_or(default_val);
    }
};

