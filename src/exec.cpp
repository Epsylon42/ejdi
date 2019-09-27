#include <cassert>

#include <exec/exec.hpp>

using namespace std;
using namespace ejdi::ast;
using namespace ejdi::exec::value;
using namespace ejdi::exec::context;

namespace ejdi::exec {
    void exec(shared_ptr<Context> ctx, const Stmt& stmt) {
        if (ast_is<Assignment>(stmt)) {
            auto assign = ast_get<Assignment>(stmt);

            if (assign->let.has_value()) {
                if (ctx->try_find_on_this_level(assign->variable.str) == nullptr) {
                    ctx->set_on_this_level(assign->variable.str, eval(ctx, assign->expr));
                } else {
                    assert("variable with the same name already exists in this scope" && 0);
                }
            } else {
                auto var = ctx->try_find(assign->variable.str);
                if (var == nullptr) {
                    assert("variable does not exist" && 0);
                }

                *var = eval(ctx, assign->expr);
            }
        } else if (ast_is<ExprStmt>(stmt)) {
            eval(ctx, ast_get<ExprStmt>(stmt)->expr);
        } else {
            assert("invalid statement value" && 0);
        }
    }

    shared_ptr<Value> eval(shared_ptr<Context> ctx, const Expr& expr) {
        if (ast_is<Variable>(expr)) {
            return ctx->find(ast_get<Variable>(expr)->variable.str);
        } else if (ast_is<Block>(expr)) {
            auto block = ast_get<Block>(expr);

            for (const auto& stmt : block->statements) {
                exec(ctx, stmt);
            }

            if (block->ret.has_value()) {
                return eval(ctx, *block->ret);
            } else {
                return make_shared<Value>(Unit{});
            }
        } else if (ast_is<FunctionCall>(expr)) {
            auto funcall = ast_get<FunctionCall>(expr);

            auto function = eval(ctx, funcall->function);
            if (holds_alternative<Function>(*function)) {
                vector<shared_ptr<Value>> args;
                args.reserve(funcall->arguments->list.size());
                for (const auto& arg : funcall->arguments->list) {
                    args.push_back(eval(ctx, arg));
                }

                return get<Function>(*function).call(move(args));
            } else {
                assert("not a function" && 0);
            }
        } else if (ast_is<StringLiteral>(expr)) {
            return make_shared<Value>(ast_get<StringLiteral>(expr)->literal.value());
        } else {
            assert("unimplemented expression" && 0);
            return nullptr;
        }
    }
}
