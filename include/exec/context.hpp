#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <tuple>

#include <span.hpp>
#include <exec/value.hpp>
#include <exec/error.hpp>

namespace ejdi::exec::context {
    struct GlobalContext;

    struct Context {
        GlobalContext& global;
        std::shared_ptr<value::Object> scope;

        error::RuntimeError error(std::string message, span::Span span = span::Span::empty()) const;
        error::RuntimeError arg_count_error(std::size_t expected, std::size_t got, span::Span = span::Span::empty()) const;
    };

    struct GlobalContext {
        std::shared_ptr<value::Object> core;
        std::unordered_map<std::string, value::Value> modules;
        std::vector<std::tuple<std::string, span::Span>> stack_trace;

        static GlobalContext with_core();

        void call(std::string name, span::Span span);
        void ret();

        Context new_module(std::string name);
    };
}
