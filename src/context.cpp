#include <cassert>
#include <iostream>

#include <exec/context.hpp>

using namespace std;
using namespace ejdi::span;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;

using Ctx = const Context&;


static Value unit() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
             Function::native_expanded(
                [](Ctx) {
                    return string("()");
                })
        );

    return obj;
}

static Value number() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
             Function::native_expanded<float>(
                [](Ctx, float val) {
                    return to_string(val);
                })
        );

    return obj;
}

static Value boolean() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
             Function::native_expanded<bool>(
                [](Ctx, bool val) {
                    return val ? string("true") : string("false");
                })
        );

    return obj;
}

static Value string_() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
             Function::native_expanded<string>(
                [](Ctx, auto val) {
                    return string(*val);
                })
        );
    obj->set("len",
             Function::native_expanded<string>(
                 [](Ctx, auto val) {
                     return (float)val->length();
                 })
        );
    obj->set("at",
             Function::native_expanded<string, float>(
                 [](Ctx, auto str, size_t index) {
                     string res;
                     res += (str->at(index));
                     return res;
                 })
        );
    obj->set("slice",
             Function::native_expanded<string, float, float>(
                 [](Ctx, auto str, size_t start, size_t end) {
                     return str->substr(start, end);
                 })
        );

    return obj;
}

static Value function_() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
            Function::native_expanded(
                [](Ctx) {
                    return string("[function]");
                })
        );

    return obj;
}

static Value object() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
            Function::native_expanded(
                [](Ctx) {
                    return string("[object]");
                })
        );

    return obj;
}


namespace ejdi::exec::context {
    GlobalContext GlobalContext::with_core() {
        auto core = make_shared<Object>();
        core->set("unit", unit());
        core->set("number", number());
        core->set("boolean", boolean());
        core->set("string", string_());
        core->set("function", function_());
        core->set("object", object());

        auto prelude = make_shared<Object>();
        prelude->set(
            "print",
            Function::native(
                [](Ctx ctx, vector<Value> args) {
                    for (auto& val : args) {
                        auto str = get_vtable(ctx, val).get("to_s").as<Function>()->call(ctx, {val});
                        cout << *str.as<string>();
                    }
                    cout.flush();

                    return Unit{};
                })
            );

        prelude->set(
            "readline",
            Function::native_expanded(
                [](Ctx) {
                    string ret;
                    getline(cin, ret);
                    return ret;
                })
            );

        prelude->set(
            "obj",
            Function::native_expanded(
                [](Ctx ctx) {
                    auto obj = make_shared<Object>();
                    obj->prototype = ctx.global.core->get("object").as<Object>();
                    return obj;
                })
            );

        core->set("prelude", move(prelude));

        return GlobalContext { move(core), {}, {} };
    }

    void GlobalContext::call(string name, Span span) {
        stack_trace.push(make_tuple(move(name), span));
    }

    void GlobalContext::ret() {
        stack_trace.pop();
    }

    Context GlobalContext::new_module(string name) {
        auto scope = make_shared<Object>();
        scope->prototype = core->get("prelude").as<Object>();
        modules.insert_or_assign(move(name), scope);
        return Context { *this, move(scope) };
    }
}
