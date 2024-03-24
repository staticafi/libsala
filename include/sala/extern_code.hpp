#ifndef SALA_EXTERN_CODE_HPP_INCLUDED
#   define SALA_EXTERN_CODE_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/exec_state.hpp>
#   include <unordered_map>
#   include <functional>
#   include <string>

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

    bool register_code(std::string const& name, std::function<void()> const& code);

    void execute();

private:
    void std_exit();
    void std_atexit();
    void std_abort(std::string const& func_name);
    void __llvm_intrinsic_bswap(std::size_t num_bytes);
    void __llvm_intrinsic_ctlz_8();
    void __llvm_intrinsic_ctlz_16();
    void __llvm_intrinsic_ctlz_32();
    void __llvm_intrinsic_ctlz_64();

    ExecState* state_;
    std::unordered_map<std::string, std::uint32_t> names2indices_;
    std::unordered_map<std::uint32_t, std::function<void()> > code_;
};


}

#endif
