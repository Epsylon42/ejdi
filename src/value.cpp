#include <exec/value.hpp>

using namespace std;

namespace ejdi::exec::value {
    Function::Function(NativeFunction func)
        : func(new NativeFunction(move(func))) {}

    shared_ptr<Value> Function::call(vector<shared_ptr<Value>> args) {
        return func->call(move(args));
    }

    shared_ptr<Value> NativeFunction::call(vector<shared_ptr<Value>> args) {
        return func(move(args));
    }
}
