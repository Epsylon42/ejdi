#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <stack>
#include <tuple>

#include <span.hpp>
#include <exec/value.hpp>

namespace ejdi::exec::context {
    struct GlobalContext;

    struct Context {
        GlobalContext& global;
        std::shared_ptr<value::Object> scope;
    };

    struct GlobalContext {
        std::shared_ptr<value::Object> core;
        std::unordered_map<std::string, value::Value> modules;
        std::stack<std::tuple<std::string, span::Span>> stack_trace;

        static GlobalContext with_core();

        void call(std::string name, span::Span span);
        void ret();

        Context new_module(std::string name);
    };
}
