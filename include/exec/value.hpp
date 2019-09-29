#pragma once

#include <functional>
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>
#include <memory>

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
    auto& __as_impl(ValueVariant& val) {
        return std::get<T>(val);
    }

    template<>
    inline auto& __as_impl<std::string>(ValueVariant& val) {
        return std::get<std::shared_ptr<std::string>>(val);
    }

    template<>
    inline auto& __as_impl<Function>(ValueVariant& val) {
        return std::get<std::shared_ptr<Function>>(val);
    }

    template<>
    inline auto& __as_impl<Object>(ValueVariant& val) {
        return std::get<std::shared_ptr<Object>>(val);
    }

    template< typename T >
    bool __is_impl(ValueVariant& val) {
        return std::holds_alternative<T>(val);
    }

    template<>
    inline bool __is_impl<std::string>(ValueVariant& val) {
        return std::holds_alternative<std::shared_ptr<std::string>>(val);
    }

    template<>
    inline bool __is_impl<Function>(ValueVariant& val) {
        return std::holds_alternative<std::shared_ptr<Function>>(val);
    }

    template<>
    inline bool __is_impl<Object>(ValueVariant& val) {
        return std::holds_alternative<std::shared_ptr<Object>>(val);
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
        decltype(auto) as() {
            return __as_impl<T>(this->value);
        }

        template< typename T >
        bool is() {
            return __is_impl<T>(this->value);
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


    Object& get_vtable(const context::Context& ctx, Value& val);


    struct IFunction;
    struct NativeFunction;

    class Function {
        std::unique_ptr<IFunction> func;

    public:
        Function(std::unique_ptr<IFunction> func) : func(std::move(func)) {}

        static Value native(NativeFunction func);

        Value call(const context::Context& ctx, std::vector<Value> args);
    };


    struct IFunction {
        virtual Value call(const context::Context& ctx, std::vector<Value> args) = 0;
    };

    struct NativeFunction : IFunction {
        std::function< Value(const context::Context&, std::vector<Value>) > func;

        template< typename F >
        NativeFunction(F func) : func(std::move(func)) {}

        Value call(const context::Context& ctx, std::vector<Value> args) override;
    };
}
