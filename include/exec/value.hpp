#pragma once

#include <functional>
#include <unordered_map>
#include <variant>
#include <string>
#include <vector>
#include <memory>

namespace ejdi::exec::value {
    struct Unit {};

    struct Value;
    class Object;
    class Function;

    using ValueVariant = std::variant<Unit, float, bool, std::shared_ptr<std::string>, std::shared_ptr<Function>, std::shared_ptr<Object>>;

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
    };

    class Object {
        std::unordered_map<std::string, Value> map;
        /*nullable*/ std::shared_ptr<Object> prototype;
        bool mutable_prototype_fields = false;

    public:
        Object(std::shared_ptr<Object> prototype = nullptr);
        static std::shared_ptr<Object> scope(std::shared_ptr<Object> parent = nullptr);

        Value& get(const std::string& name);
        /*nullable*/ Value* try_get(const std::string& name);
        /*nullable*/ Value* try_get_no_prototype(const std::string& name);
        void set(std::string name, Value value);
        void set_no_prototype(std::string name, Value value);
    };




    struct IFunction;
    struct NativeFunction;

    class Function {
        std::unique_ptr<IFunction> func;

    public:
        Function(std::unique_ptr<IFunction> func) : func(std::move(func)) {}

        static Value native(NativeFunction func);

        Value call(std::vector<Value> args);
    };


    struct IFunction {
        virtual Value call(std::vector<Value> args) = 0;
    };

    struct NativeFunction : IFunction {
        std::function< Value(std::vector<Value>) > func;

        template< typename F >
        NativeFunction(F func) : func(std::move(func)) {}

        Value call(std::vector<Value> args) override;
    };
}
