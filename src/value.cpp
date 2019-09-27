#include <cassert>

#include <exec/value.hpp>

using namespace std;

namespace ejdi::exec::value {
    Object::Object(shared_ptr<Object> prototype)
        : prototype(move(prototype)) {}

    shared_ptr<Object> Object::scope(shared_ptr<Object> parent) {
        auto scope = make_shared<Object>(move(parent));
        scope->mutable_prototype_fields = true;

        return scope;
    }

    Value* Object::try_get_no_prototype(const string& name) {
        auto iter = map.find(name);
        if (iter != map.end()) {
            return &iter->second;
        } else {
            return nullptr;
        }
    }

    Value* Object::try_get(const string& name) {
        auto ptr = try_get_no_prototype(name);
        if (ptr == nullptr && prototype != nullptr) {
            return prototype->try_get(name);
        } else {
            return ptr;
        }
    }

    Value& Object::get(const string& name) {
        auto ptr = try_get(name);
        if (ptr != nullptr) {
            return *ptr;
        } else {
            assert("field not found" && 0);
        }
    }

    void Object::set_no_prototype(string name, Value value) {
        map.insert_or_assign(move(name), move(value));
    }

    void Object::set(string name, Value value) {
        auto ptr = try_get_no_prototype(name);
        if (ptr != nullptr) {
            *ptr = move(value);
            return;
        } else if (mutable_prototype_fields && prototype != nullptr) {
            ptr = prototype->try_get(name);
            if (ptr != nullptr) {
                *ptr = move(value);
                return;
            }
        }

        map.insert_or_assign(move(name), move(value));
    }



    Value Function::native(NativeFunction func) {
        return make_shared<Function>(unique_ptr<IFunction>(new NativeFunction(move(func))));
    }

    Value Function::call(vector<Value> args) {
        return func->call(move(args));
    }

    Value NativeFunction::call(vector<Value> args) {
        return func(move(args));
    }
}
