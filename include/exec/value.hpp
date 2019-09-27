#pragma once

#include <functional>
#include <variant>
#include <string>
#include <vector>
#include <memory>

namespace ejdi::exec::value {
    struct Unit {};

    class Function;
    struct IFunction;
    struct NativeFunction;

    using Value = std::variant<float, bool, std::string, Unit, Function>;

    class Function {
        std::unique_ptr<IFunction> func;

    public:
        Function(NativeFunction func);

        std::shared_ptr<Value> call(std::vector<std::shared_ptr<Value>> args);
    };

    struct IFunction {
        virtual std::shared_ptr<Value> call(std::vector<std::shared_ptr<Value>> args) = 0;
    };

    struct NativeFunction : IFunction {
        std::function< std::shared_ptr<Value>(std::vector<std::shared_ptr<Value>>) > func;

        std::shared_ptr<Value> call(std::vector<std::shared_ptr<Value>> args) override;
    };
}
