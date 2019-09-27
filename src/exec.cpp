#include <cassert>

#include <exec/exec.hpp>

using namespace std;
using namespace ejdi::ast;
using namespace ejdi::exec::value;

namespace ejdi::exec {
    void exec(const shared_ptr<Object>& scope, const Stmt& stmt) {
        if (ast_is<Assignment>(stmt)) {
            auto assign = ast_get<Assignment>(stmt);

            if (assign->let.has_value()) {
                if (scope->try_get_no_prototype(assign->variable.str) == nullptr) {
                    scope->set_no_prototype(assign->variable.str, eval(scope, assign->expr));
                } else {
                    assert("variable with the same name already exists in this scope" && 0);
                }
            } else {
                auto var = scope->try_get(assign->variable.str);
                if (var == nullptr) {
                    assert("variable does not exist" && 0);
                }

                *var = eval(scope, assign->expr);
            }
        } else if (ast_is<ExprStmt>(stmt)) {
            eval(scope, ast_get<ExprStmt>(stmt)->expr);
        } else {
            assert("invalid statement value" && 0);
        }
    }

    Value eval(const shared_ptr<Object>& scope, const Expr& expr) {
        if (ast_is<Variable>(expr)) {
            return scope->get(ast_get<Variable>(expr)->variable.str);
        } else if (ast_is<Block>(expr)) {
            auto block = ast_get<Block>(expr);

            for (const auto& stmt : block->statements) {
                exec(scope, stmt);
            }

            if (block->ret.has_value()) {
                return eval(scope, *block->ret);
            } else {
                return Unit{};
            }
        } else if (ast_is<FunctionCall>(expr)) {
            auto funcall = ast_get<FunctionCall>(expr);

            auto function = eval(scope, funcall->function);
            if (holds_alternative<shared_ptr<Function>>(function.value)) {
                vector<Value> args;
                args.reserve(funcall->arguments->list.size());
                for (const auto& arg : funcall->arguments->list) {
                    args.push_back(eval(scope, arg));
                }

                return get<shared_ptr<Function>>(function.value)->call(move(args));
            } else {
                assert("not a function" && 0);
            }
        } else if (ast_is<StringLiteral>(expr)) {
            return ast_get<StringLiteral>(expr)->literal.value();
        } else {
            assert("unimplemented expression" && 0);
        }
    }
}
