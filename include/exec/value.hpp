#pragma once

#include <cassert>
#include <functional>
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>
#include <memory>

#include <exec/error.hpp>

namespace ejdi::exec::context {
    struct Context;
}

namespace ejdi::exec::value {
    struct Unit {};

    struct Value;
    class Object;
    class Function;

    using ValueVariant = std::variant<Unit, float, bool, std::shared_ptr<std::string>, std::shared_ptr<Function>, std::shared_ptr<Object>>;

    template< typename T >
    struct _WrappedRc {
        using TYPE = T;
    };
    template<>
    struct _WrappedRc<std::string> {
        using TYPE = std::shared_ptr<std::string>;
    };
    template<>
    struct _WrappedRc<Function> {
        using TYPE = std::shared_ptr<Function>;
    };
    template<>
    struct _WrappedRc<Object> {
        using TYPE = std::shared_ptr<Object>;
    };
    template< typename T >
    using WrappedRc = _WrappedRc<T>::TYPE;

    inline std::string_view __type_name(Unit*) {
        return "unit";
    }
    inline std::string_view __type_name(float*) {
        return "number";
    }
    inline std::string_view __type_name(bool*) {
        return "boolean";
    }
    inline std::string_view __type_name(std::string*) {
        return "string";
    }
    inline std::string_view __type_name(Function*) {
        return "function";
    }
    inline std::string_view __type_name(Object*) {
        return "object";
    }
    inline std::string_view __type_name(std::shared_ptr<std::string>*) {
        return "string";
    }
    inline std::string_view __type_name(std::shared_ptr<Function>*) {
        return "function";
    }
    inline std::string_view __type_name(std::shared_ptr<Object>*) {
        return "object";
    }
    template< typename T >
    inline std::string_view type_name() {
        return __type_name(static_cast<T*>(nullptr));
    }

    struct Value {
        ValueVariant value;

        Value(Value&& other) = default;
        Value(const Value& other) = default;

        Value& operator=(const Value& other) = default;

        Value(ValueVariant val) : value(std::move(val)) {}
        Value(Unit val) : value(val) {}
        Value(float val) : value(val) {}
        Value(bool val) : value(val) {}
        Value(std::string val) : value(std::make_shared<std::string>(std::move(val))) {}
        Value(std::shared_ptr<std::string> val) : value(std::move(val)) {}
        Value(std::shared_ptr<Function> val) : value(std::move(val)) {}
        Value(std::shared_ptr<Object> val) : value(std::move(val)) {}

        template< typename T >
        bool is() {
            return std::holds_alternative<WrappedRc<T>>(this->value);
        }

        template< typename T >
        auto& as() {
            if (is<T>()) {
                return std::get<WrappedRc<T>>(this->value);
            } else {
                std::string msg = "wrong type: expected ";
                msg += type_name<T>();
                msg += ", got ";
                msg += std::visit([](auto& arg){ return __type_name(&arg); }, value);
                throw error::RuntimeError { std::move(msg) };
            }
        }

    };

    struct Object {
        std::unordered_map<std::string, Value> map;
        /*nullable*/ std::shared_ptr<Object> prototype;
        bool mutable_prototype_fields = false;

        Object(std::shared_ptr<Object> prototype = nullptr);
        static std::shared_ptr<Object> scope(std::shared_ptr<Object> parent = nullptr);

        Value& get(const std::string& name);
        /*nullable*/ Value* try_get(const std::string& name);
        /*nullable*/ Value* try_get_no_prototype(const std::string& name);
        void set(std::string name, Value value);
        void set_no_prototype(std::string name, Value value);
    };


    Object& get_vtable(context::Context& ctx, Value& val);


    struct IFunction;
    struct NativeFunction;

    template< std::size_t... Is >
    auto values_tuple(std::vector<Value>& args, std::integer_sequence<std::size_t, Is...>) {
        return std::make_tuple(std::move(args[Is])...);
    }


    class Function {
        std::unique_ptr<IFunction> func;

    public:
        Function(std::unique_ptr<IFunction> func) : func(std::move(func)) {}

        static Value native(NativeFunction func);

        template< typename... Args, typename F >
        static Value native_expanded(F func) {
            auto wrapper = [func{std::move(func)}](context::Context& ctx, std::vector<Value> args) {
                if (args.size() < sizeof...(Args)) {
                    std::string msg = "not enough arguments: needs at least ";
                    msg += std::to_string(sizeof...(Args));
                    msg += ", got ";
                    msg += std::to_string(args.size());
                    throw error::RuntimeError { std::move(msg) };
                }

                auto seq = std::index_sequence_for<Args...>();
                return std::apply([&](auto&&... args) {
                    return func(ctx, std::move(args.template as<Args>())...);
                }, values_tuple(args, seq));
            };

            return native(std::move(wrapper));
        }

        Value call(context::Context& ctx, std::vector<Value> args);
    };


    struct IFunction {
        virtual Value call(context::Context& ctx, std::vector<Value> args) = 0;
    };

    struct NativeFunction : IFunction {
        std::function< Value(context::Context&, std::vector<Value>) > func;

        template< typename F >
        NativeFunction(F func) : func(std::move(func)) {}

        Value call(context::Context& ctx, std::vector<Value> args) override;
    };
}
