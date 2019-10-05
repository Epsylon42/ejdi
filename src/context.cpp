#include <iostream>
#include <cmath>

#include <exec/context.hpp>

using namespace std;
using namespace ejdi::span;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;
using namespace ejdi::exec::error;

using Ctx = Context&;


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
                    auto str = to_string(val);

                    if (str.find('.') != string::npos) {
                        while (str.back() == '0') {
                            str.pop_back();
                        }
                        if (str.back() == '.') {
                            str.pop_back();
                        }
                    }

                    return str;
                })
        );
    obj->set("ceil",
             Function::native_expanded<float>(
                 [](Ctx, float val) {
                     return ceil(val);
                 })
        );
    obj->set("floor",
             Function::native_expanded<float>(
                 [](Ctx, float val) {
                     return floor(val);
                 })
        );
    obj->set("round",
             Function::native_expanded<float>(
                 [](Ctx, float val) {
                     return round(val);
                 })
        );
    obj->set("pow",
             Function::native_expanded<float, float>(
                 [](Ctx, float val, float power) {
                     return pow(val, power);
                 })
        );
    obj->set("chr",
             Function::native_expanded<float>(
                 [](Ctx, float val) {
                     string ret;
                     ret += char(val);
                     return ret;
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
    obj->set("to_n",
             Function::native_expanded<string>(
                 [](Ctx, auto val) -> Value {
                     try {
                         return stof(*val);
                     } catch (...) {
                         return Unit{};
                     }
                 })
        );
    obj->set("ord",
             Function::native_expanded<string>(
                 [](Ctx, auto val) {
                     if (!val->empty()) {
                         return float(int(val->at(0)));
                     } else {
                         return 0.0f;
                     }
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
                 [](Ctx ctx, auto str, size_t index) {
                     string res;
                     if (str->length() > index) {
                         res += (str->at(index));
                         return res;
                     } else {
                         throw ctx.error("string index out of range");
                     }
                 })
        );
    obj->set("slice",
             Function::native_expanded<string, float, float>(
                 [](Ctx ctx, auto str, size_t start, size_t end) {
                     if (start > end) {
                         throw ctx.error("start index is higher than end index");
                     } else if (str->length() <= start || str->length() < end) {
                         throw ctx.error("string index out of range");
                     }
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

static Value array_() {
    auto obj = make_shared<Object>();
    obj->set("to_s",
             Function::native_expanded<Array>(
                 [](Ctx ctx, auto arr) {
                     string res = "[";
                     for (auto& elem : *arr) {
                         res += *get_vtable(ctx, elem)
                             .getf("to_s")
                             .call(ctx, { elem })
                             .template as<string>();

                         res += ", ";
                     }
                     if (!arr->empty()) {
                         res.pop_back();
                         res.pop_back();
                     }
                     res += ']';

                     return res;
                 })
        );
    obj->set("push",
             Function::native(
                 [](Ctx ctx, vector<Value> val) {
                     if (val.size() == 0) {
                         throw ctx.arg_count_error(1, 0);
                     }

                     auto& arr = val[0].as<Array>();

                     for (auto iter = next(val.begin()); iter != val.end(); ++iter) {
                         arr->push_back(move(*iter));
                     }

                     return Unit{};
                 })
        );
    obj->set("pop",
             Function::native_expanded<Array>(
                 [](Ctx, auto arr) -> Value {
                     if (arr->empty()) {
                         return Unit{};
                     }

                     auto ret = move(arr->back());
                     arr->pop_back();
                     return ret;
                 })
        );
    obj->set("at",
             Function::native_expanded<Array, float>(
                 [](Ctx ctx, auto arr, size_t index) {
                     if (arr->size() < index) {
                         throw ctx.error("array index out of bounds");
                     }

                     return (*arr)[index];
                 })
        );

    return obj;
}


namespace ejdi::exec::context {
    RuntimeError Context::error(string message, Span span) const {
        return RuntimeError { move(message), span, global.stack_trace };
    }

    RuntimeError Context::arg_count_error(size_t expected, size_t got, Span span) const {
        string msg = "not enough arguments: need at least ";
        msg += to_string(expected);
        msg += ", got ";
        msg += to_string(got);

        return error(move(msg), span);
    }

    GlobalContext GlobalContext::with_core() {
        auto core = make_shared<Object>();

        vector<tuple<string, function<Value()>>> prototypes = {
            { "Unit", unit },
            { "Number", number },
            { "Boolean", boolean },
            { "String", string_ },
            { "Function", function_ },
            { "Object", object },
            { "Array", array_ }
        };

        auto prelude = make_shared<Object>();

        for (auto& [ name, func ] : prototypes) {
            auto proto = func();
            core->set(name, proto);
            prelude->set(name, proto);
        }

        prelude->set(
            "print",
            Function::native(
                [](Ctx ctx, vector<Value> args) {
                    for (auto& val : args) {
                        auto str = get_vtable(ctx, val).getf("to_s").call(ctx, {val});
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
        stack_trace.push_back(make_tuple(move(name), span));
    }

    void GlobalContext::ret() {
        stack_trace.pop_back();
    }

    Context GlobalContext::new_module(string name) {
        auto scope = make_shared<Object>();
        scope->prototype = core->get("prelude").as<Object>();
        modules.insert_or_assign(move(name), scope);
        return Context { *this, move(scope) };
    }
}
