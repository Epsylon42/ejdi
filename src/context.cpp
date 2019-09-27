#include <cassert>

#include <exec/context.hpp>

using namespace std;
using namespace ejdi::exec::value;

namespace ejdi::exec::context {
    shared_ptr<Value>* Context::try_find(const string& name) {
        auto iter = variables.find(name);
        if (iter != variables.end()) {
            return &iter->second;
        } else {
            if (parent == nullptr) {
                return nullptr;
            } else {
                return parent->try_find(name);
            }
        }
    }

    shared_ptr<Value>& Context::find(const string& name) {
        auto ptr = try_find(name);
        if (ptr != nullptr) {
            return *ptr;
        } else {
            assert("variable does not exist" && 0);
        }
    }

    shared_ptr<Value>* Context::try_find_on_this_level(const string& name) {
        auto iter = variables.find(name);
        if (iter != variables.end()) {
            return &iter->second;
        } else {
            return nullptr;
        }
    }

    void Context::set_on_this_level(string name, shared_ptr<Value> val) {
        variables.insert_or_assign(move(name), move(val));
    }
}
