#include <sala/extern_code.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>

namespace sala {


ExternCode::ExternCode(ExecState* const state)
    : state_{ state }
    , names2indices_{}
    , code_{}
{
    for (auto idx : program().external_functions())
        names2indices_.insert({ program().functions().at(idx).name(), idx });

    register_code("exit", [this](){ this->std_exit(); });
    register_code("atexit", [this](){ this->std_atexit(); });
    register_code("abort", [this](){ this->std_abort("abort"); });
    // POSIX:
    register_code("__assert_fail", [this](){ this->std_abort("__assert_fail"); });
}


bool ExternCode::register_code(std::string const& name, std::function<void()> const& code)
{
    auto it = names2indices_.find(name);
    if (it == names2indices_.end())
        return false;
    return code_.insert({ it->second, code }).second;
}


void ExternCode::execute()
{
    auto it = code_.find(function().index());
    if (it != code_.end())
        it->second();
}


void ExternCode::std_exit()
{
    int const exit_code{ parameters().front().as<int>() };

    state().set_stage(ExecState::Stage::TERMINATING);
    state().set_termination(
        ExecState::Termination::NORMAL,
        "test_interpreter[extern_code]",
        "Called exit(" + std::to_string(exit_code) + ")."
        );
    state().set_exit_code(exit_code);

    state().set_stack_exit_depth(state().stack_segment().size());
}


void ExternCode::std_atexit()
{
    MemPtr const func_ptr{ parameters().back().as<MemPtr>() };
    auto const it = state().functions_at_addresses().find(func_ptr);
    if (it == state().functions_at_addresses().end())
        state().set_termination(
            ExecState::Termination::ERROR,
            "test_interpreter[extern_code]",
            "Called atexit() with an invalid pointer. No function was pushed."
            );
    else
        state().push_atexit_function(it->second);
}


void ExternCode::std_abort(std::string const& func_name)
{
    state().set_stage(ExecState::Stage::FINISHED);
    state().set_termination(
        ExecState::Termination::ERROR,
        "test_interpreter[extern_code]",
        "Called " + func_name + "().");
    state().set_exit_code(0);
}


}
