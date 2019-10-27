#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <tuple>
#include <filesystem>

#include <span.hpp>
#include <linemap.hpp>
#include <exec/value.hpp>
#include <exec/error.hpp>

namespace ejdi::exec::context {
    struct GlobalContext;

    struct Context {
        GlobalContext& global;
        std::shared_ptr<value::Object> scope;
        std::filesystem::path module_path;
        std::vector<std::tuple<std::string, span::Span>> stack_trace;

        error::RuntimeError error(std::string message, span::Span span = span::Span::empty()) const;
        error::RuntimeError arg_count_error(std::size_t expected, std::size_t got, span::Span = span::Span::empty()) const;

        Context child();
    };

    struct GlobalContext {
        std::shared_ptr<value::Object> core;

        std::unordered_map<std::string, std::shared_ptr<value::Object>> modules;
        std::unordered_map<std::string, linemap::Linemap> linemaps;
        std::vector<std::filesystem::path> global_import_paths;

        static GlobalContext with_core();

        value::Value load_module(std::string_view module, Context* loading_from = nullptr);
        void print_error_message(const error::RuntimeError& error) const;

        std::shared_ptr<value::Object> new_module(std::string name);
    };
}
