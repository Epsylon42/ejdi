#include <cassert>
#include <iostream>

#include <exec/value.hpp>
#include <exec/context.hpp>
#include <exec/exec.hpp>

using namespace std;
using namespace ejdi::exec::context;

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
            throw error::RuntimeError { string("field '" + name + "' not found") };
        }
    }

    Function& Object::getf(const string& name) {
        return *get(name).as<Function>();
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

    Value Function::lang(LangFunction func) {
        return make_shared<Function>(unique_ptr<IFunction>(new LangFunction(move(func))));
    }

    Value Function::call(Context& ctx, vector<Value> args) {
        return func->call(ctx, move(args));
    }


    Value NativeFunction::call(Context& ctx, vector<Value> args) {
        return func(ctx, move(args));
    }


    Value LangFunction::call(Context& ctx, vector<Value> args) {
        auto func_ctx = ctx.child();

        for (size_t i = 0; i < argnames->list.size(); i++) {
            const string& name = argnames->list[i].str;

            if (i < args.size()) {
                func_ctx.scope->set_no_prototype(name, move(args[i]));
            } else {
                func_ctx.scope->set_no_prototype(name, Unit{});
            }
        }

        return eval(func_ctx, body);
    }


    Object& get_vtable(Context& ctx, Value& val) {
#define VTABLE(table) (*ctx.global.core->get(table).as<Object>())

        if (val.is<Unit>()) {
            return VTABLE("Unit");
        } else if (val.is<float>()) {
            return VTABLE("Number");
        } else if (val.is<bool>()) {
            return VTABLE("Boolean");
        } else if (val.is<string>()) {
            return VTABLE("String");
        } else if (val.is<Function>()) {
            return VTABLE("Function");
        } else if (val.is<Array>()) {
            return VTABLE("Array");
        } else {
            return *val.as<Object>();
        }

#undef VTABLE
    }
}
