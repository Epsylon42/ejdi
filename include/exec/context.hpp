#pragma once

#include <string>
#include <unordered_map>

#include <exec/value.hpp>

namespace ejdi::exec::context {
    class Context {
        std::unordered_map<std::string, std::shared_ptr<value::Value>> variables;

        Context* parent;

    public:
        Context(Context* parent = nullptr) : parent(parent) {}

        std::shared_ptr<value::Value>& find(const std::string& name);
        std::shared_ptr<value::Value>* try_find(const std::string& name);
        void set_on_this_level(std::string name, std::shared_ptr<value::Value> val);
        std::shared_ptr<value::Value>* try_find_on_this_level(const std::string& name);
    };
}
