#include <sala/extern_code.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>

namespace sala {


template<typename T>
static T __llvm_intrinsic_ctlz_impl(T const value)
{
    T constexpr n = (T)(8U * sizeof(T));
    T i = (T)0;
    T bit_check{ (T)(1 << (sizeof(T) - 1)) };
    while ((value & bit_check) == 0 && i != n)
    {
        ++i;
        bit_check >>= 1;
    }
    return i;
}


ExternCode::ExternCode(ExecState* const state)
    : state_{ state }
    , code_{}
{
    REGISTER_EXTERN_CODE(exit, this->std_exit() );
    REGISTER_EXTERN_CODE(atexit, this->std_atexit() );
    REGISTER_EXTERN_CODE(abort, this->std_abort("abort") );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_bswap_8, this->__llvm_intrinsic_bswap(8ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_bswap_16, this->__llvm_intrinsic_bswap(16ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_bswap_32, this->__llvm_intrinsic_bswap(32ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_bswap_64, this->__llvm_intrinsic_bswap(64ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_ctlz_8, this->__llvm_intrinsic_ctlz_8() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_ctlz_16, this->__llvm_intrinsic_ctlz_16() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_ctlz_32, this->__llvm_intrinsic_ctlz_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic_ctlz_64, this->__llvm_intrinsic_ctlz_64() );
    // POSIX:
    REGISTER_EXTERN_CODE(__assert_fail, this->std_abort("__assert_fail") );
}


void ExternCode::register_code(std::string const& function_name, std::function<void()> const& code)
{
    code_.insert_or_assign(function_name, code);
}


void ExternCode::call_code_of_current_function_if_registered_external()
{
    if (function().is_external())
    {
        auto it = code_.find(function().name());
        if (it != code_.end())
            it->second();
    }
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


void ExternCode::__llvm_intrinsic_bswap(std::size_t const num_bytes)
{
    auto const dst_ptr{ parameters().front().as<MemPtr>() };
    auto const src_ptr{ parameters().back().start() };
    for (std::size_t i = 0ULL; i != num_bytes; ++i)
        *(dst_ptr + num_bytes - (i + 1ULL)) = *(src_ptr + i);
}


void ExternCode::__llvm_intrinsic_ctlz_8()
{
    parameters().front().as_ref<std::uint8_t>() = __llvm_intrinsic_ctlz_impl(parameters().at(1).as<std::uint8_t>());
}


void ExternCode::__llvm_intrinsic_ctlz_16()
{
    parameters().front().as_ref<std::uint16_t>() = __llvm_intrinsic_ctlz_impl(parameters().at(1).as<std::uint16_t>());
}


void ExternCode::__llvm_intrinsic_ctlz_32()
{
    parameters().front().as_ref<std::uint32_t>() = __llvm_intrinsic_ctlz_impl(parameters().at(1).as<std::uint32_t>());
}


void ExternCode::__llvm_intrinsic_ctlz_64()
{
    parameters().front().as_ref<std::uint64_t>() = __llvm_intrinsic_ctlz_impl(parameters().at(1).as<std::uint64_t>());
}


}
