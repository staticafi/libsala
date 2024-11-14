#ifndef SALA_EXTERN_CODE_HPP_INCLUDED
#   define SALA_EXTERN_CODE_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/exec_state.hpp>
#   include <unordered_map>
#   include <functional>
#   include <string>

#   define REGISTER_EXTERN_CODE(FN_NAME, IMPL) register_code(#FN_NAME, [this]() { IMPL; })


namespace sala {


struct ExternCode
{
    explicit ExternCode(ExecState* const state);
    virtual ~ExternCode() {}

    Program const& program() const { return state().program(); }
    Function const& function() const { return state().current_function(); }
    std::vector<MemBlock> const& parameters() { return state().stack_top().parameters(); }

    ExecState const& state() const { return *state_; }
    ExecState& state() { return *state_; }

    void register_code(std::string const& function_name, std::function<void()> const& code);

    void call_code_of_current_function_if_registered_external();

    Instruction const* get_call_instruction() const;

private:
    void std_exit();
    void std_atexit();
    void std_abort(std::string const& func_name);
    void __llvm_intrinsic__bswap(std::size_t num_bytes);
    void __llvm_intrinsic__ctlz_8();
    void __llvm_intrinsic__ctlz_16();
    void __llvm_intrinsic__ctlz_32();
    void __llvm_intrinsic__ctlz_64();
    void __llvm_intrinsic__trunc_32();
    void __llvm_intrinsic__trunc_64();
    void __llvm_intrinsic__ceil_32();
    void __llvm_intrinsic__ceil_64();
    void __llvm_intrinsic__floor_32();
    void __llvm_intrinsic__floor_64();
    void __llvm_intrinsic__rint_32();
    void __llvm_intrinsic__rint_64();
    void __llvm_intrinsic__is_fpclass_32();
    void __llvm_intrinsic__is_fpclass_64();

    ExecState* state_;
    std::unordered_map<std::string, std::function<void()> > code_;
};


}

#endif
