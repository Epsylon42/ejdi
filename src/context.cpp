#include <iostream>
#include <cmath>
#include <fstream>

#include <exec/context.hpp>
#include <exec/exec.hpp>
#include <util.hpp>
#include <lexer.hpp>
#include <lexem_groups.hpp>
#include <ast.hpp>
#include <parser.hpp>
#include <span.hpp>

using namespace std;
using namespace ejdi::span;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;
using namespace ejdi::exec::error;
using namespace ejdi::util;

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
    obj->set("len",
             Function::native_expanded<Array>(
                 [](Ctx, auto arr) {
                     return (float)arr->size();
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
    obj->set("set",
             Function::native_expanded<Array, float, Value>(
                 [](Ctx ctx, auto arr, size_t index, Value val) {
                     if (arr->size() < index) {
                         throw ctx.error("array index out of bounds");
                     }

                     (*arr)[index] = move(val);

                     return Unit{};
                 })
        );

    return obj;
}

static Value iter() {
    auto obj = make_shared<Object>();
    obj->set("end", make_shared<Object>());
    return obj;
}


namespace ejdi::exec::context {
    RuntimeError Context::error(string message, Span span) const {
        return RuntimeError { move(message), span, stack_trace };
    }

    RuntimeError Context::arg_count_error(size_t expected, size_t got, Span span) const {
        string msg = "not enough arguments: need at least ";
        msg += to_string(expected);
        msg += ", got ";
        msg += to_string(got);

        return error(move(msg), span);
    }

    Context Context::child() {
        auto child_scope = make_shared<Object>();
        child_scope->prototype = scope;
        child_scope->mutable_prototype_fields = true;

        return Context { global, move(child_scope) };
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
            { "Array", array_ },
            { "Iterator", iter }
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
                    obj->prototype = ctx.global.core->get("Object").as<Object>();
                    return obj;
                })
            );

        prelude->set(
            "require",
            Function::native_expanded<string>(
                [](Ctx ctx, auto str) {
                    return ctx.global.load_module(*str);
                })
            );

        core->set("prelude", move(prelude));

        return GlobalContext { move(core), {}, {} };
    }

    shared_ptr<Object> GlobalContext::new_module(string name) {
        auto mod = make_shared<Object>();
        mod->prototype = core->get("prelude").as<Object>();
        modules.insert_or_assign(move(name), mod);
        return mod;
    }

    Value GlobalContext::load_module(string_view module, Context* loading_from) {
        using namespace std::filesystem;

        auto stack_trace = [&]() -> decltype(loading_from->stack_trace) {
            if (loading_from == nullptr) {
                return {};
            } else {
                return loading_from->stack_trace;
            }
        };

        path module_path;
        if (starts_with(module, "./")) {
            module_path = module;
        } else {
            for (const auto& global_import_path : global_import_paths) {
                module_path = relative(module, global_import_path);
                if (exists(module_path) && !is_directory(module_path)) {
                    break;
                }
            }
            module_path = module;
        }

        auto maybe_module = modules.find(module_path);
        if (maybe_module != modules.end()) {
            return maybe_module->second->get("exports");
        }

        if (!exists(module_path)) {
            string msg = "Could not find module ";
            msg += module;
            throw RuntimeError{ move(msg), Span::empty(), stack_trace() };
        }
        if (is_directory(module_path)) {
            string msg = "Module ";
            msg += module;
            msg += " is a directory";
            throw RuntimeError { move(msg), Span::empty(), stack_trace() };
        }

        try {
            auto file = ifstream(module_path);
            string source { istreambuf_iterator<char>(file), {}};

            //TODO: use proper file paths
            auto lexems = lexer::actions::split_string(source, "___");
            auto group = lexer::groups::find_groups(move(lexems));
            auto program = parser::parse_program(*group);
            if (!program.has_result()) {
                throw move(program.error());
            }
            auto mod = new_module(module_path);
            mod->set("exports", Unit{});
            auto ctx = Context { *this, move(mod) };
            exec::exec_program(ctx, *program.get());
            return ctx.scope->get("exports");
        } catch (logic_error& e) {
            throw RuntimeError { e.what(), Span::empty(), stack_trace() };
        } catch (parser::result::ParserError& e) {
            string msg = "Parser error: expected ";
            msg += e.expected;
            msg += " but got ";
            msg += e.got;
            throw RuntimeError { move(msg), e.span, stack_trace() };
        }
    }

    void GlobalContext::print_error_message(const RuntimeError& error) const {
        cerr << error.root_error << endl;
    }
}
