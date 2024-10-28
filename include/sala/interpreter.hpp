#ifndef SALA_INTERPRETER_HPP_INCLUDED
#   define SALA_INTERPRETER_HPP_INCLUDED

#   include <sala/exec_state.hpp>
#   include <sala/instr_switch.hpp>
#   include <sala/extern_code.hpp>
#   include <sala/analyzer.hpp>
#   include <vector>
#   include <functional>

namespace sala {


struct Interpreter : public InstrSwitch
{
    explicit Interpreter(ExecState* state, ExternCode* extern_code, std::vector<Analyzer*> const& analyzers = {});

    ExecState const& state() const { return *state_; }
    ExecState& state() { return *state_; }

    Program const& program() const { return state().program(); }
    Function const& function() const { return state().current_function(); }
    BasicBlock const& block() const { return state().current_block(); }
    Instruction const& instruction() const override { return state().current_instruction(); }
    std::vector<MemBlock const*> const& operands() const override { return state().current_operands(); }
    InstrPointer const& ip() const { return state().stack_top().ip(); }
    InstrPointer& ip() { return state().stack_top().ip(); }

    ExternCode const& extern_code() const { return *extern_code_; }
    ExternCode& extern_code() { return *extern_code_; }
    std::vector<Analyzer*> const& analyzers() const { return analyzers_; }

    std::uint64_t  num_steps() const { return num_steps_; }

    bool done() const { return state().stage() == ExecState::Stage::FINISHED; }
    void step();

    void run();
    void run(double max_seconds);
    void run(std::function<bool(std::string&)> const&  terminator);

private:

    void do_halt() override;

    void do_address() override;
    void do_load() override;
    void do_store() override;

    void do_copy_8() override;
    void do_copy_16() override;
    void do_copy_32() override;
    void do_copy_64() override;
    void do_copy() override;

    void do_memcpy() override;
    void do_memmove() override;
    void do_memset() override;
    void do_moveptr() override;

    void do_alloca() override;
    void do_stacksave() override;
    void do_stackrestore() override;
    void do_malloc() override;
    void do_free() override;

    void do_add_s8() override;
    void do_add_s16() override;
    void do_add_s32() override;
    void do_add_s64() override;
    void do_add_u8() override;
    void do_add_u16() override;
    void do_add_u32() override;
    void do_add_u64() override;
    void do_add_f32() override;
    void do_add_f64() override;

    void do_sub_s8() override;
    void do_sub_s16() override;
    void do_sub_s32() override;
    void do_sub_s64() override;
    void do_sub_u8() override;
    void do_sub_u16() override;
    void do_sub_u32() override;
    void do_sub_u64() override;
    void do_sub_f32() override;
    void do_sub_f64() override;

    void do_mul_s8() override;
    void do_mul_s16() override;
    void do_mul_s32() override;
    void do_mul_s64() override;
    void do_mul_u8() override;
    void do_mul_u16() override;
    void do_mul_u32() override;
    void do_mul_u64() override;
    void do_mul_f32() override;
    void do_mul_f64() override;

    void do_div_s8() override;
    void do_div_s16() override;
    void do_div_s32() override;
    void do_div_s64() override;
    void do_div_u8() override;
    void do_div_u16() override;
    void do_div_u32() override;
    void do_div_u64() override;
    void do_div_f32() override;
    void do_div_f64() override;

    void do_rem_s8() override;
    void do_rem_s16() override;
    void do_rem_s32() override;
    void do_rem_s64() override;
    void do_rem_u8() override;
    void do_rem_u16() override;
    void do_rem_u32() override;
    void do_rem_u64() override;

    void do_and_8() override;
    void do_and_16() override;
    void do_and_32() override;
    void do_and_64() override;

    void do_or_8() override;
    void do_or_16() override;
    void do_or_32() override;
    void do_or_64() override;

    void do_xor_8() override;
    void do_xor_16() override;
    void do_xor_32() override;
    void do_xor_64() override;

    void do_shl_8() override;
    void do_shl_16() override;
    void do_shl_32() override;
    void do_shl_64() override;

    void do_shr_s8() override;
    void do_shr_s16() override;
    void do_shr_s32() override;
    void do_shr_s64() override;
    void do_shr_u8() override;
    void do_shr_u16() override;
    void do_shr_u32() override;
    void do_shr_u64() override;

    void do_neg_f32() override;
    void do_neg_f64() override;

    void do_extend_s8_s16() override;
    void do_extend_s8_s32() override;
    void do_extend_s8_s64() override;
    void do_extend_s16_s32() override;
    void do_extend_s16_s64() override;
    void do_extend_s32_s64() override;
    void do_extend_u8_u16() override;
    void do_extend_u8_u32() override;
    void do_extend_u8_u64() override;
    void do_extend_u16_u32() override;
    void do_extend_u16_u64() override;
    void do_extend_u32_u64() override;
    void do_extend_f32_f64() override;

    void do_truncate_u64_u32() override;
    void do_truncate_u64_u16() override;
    void do_truncate_u64_u8() override;
    void do_truncate_u32_u16() override;
    void do_truncate_u32_u8() override;
    void do_truncate_u16_u8() override;
    void do_truncate_f64_f32() override;

    void do_f2i_f32_s8() override;
    void do_f2i_f32_s16() override;
    void do_f2i_f32_s32() override;
    void do_f2i_f32_s64() override;
    void do_f2i_f32_u8() override;
    void do_f2i_f32_u16() override;
    void do_f2i_f32_u32() override;
    void do_f2i_f32_u64() override;
    void do_f2i_f64_s8() override;
    void do_f2i_f64_s16() override;
    void do_f2i_f64_s32() override;
    void do_f2i_f64_s64() override;
    void do_f2i_f64_u8() override;
    void do_f2i_f64_u16() override;
    void do_f2i_f64_u32() override;
    void do_f2i_f64_u64() override;

    void do_i2f_s8_f32() override;
    void do_i2f_s8_f64() override;
    void do_i2f_s16_f32() override;
    void do_i2f_s16_f64() override;
    void do_i2f_s32_f32() override;
    void do_i2f_s32_f64() override;
    void do_i2f_s64_f32() override;
    void do_i2f_s64_f64() override;
    void do_i2f_u8_f32() override;
    void do_i2f_u8_f64() override;
    void do_i2f_u16_f32() override;
    void do_i2f_u16_f64() override;
    void do_i2f_u32_f32() override;
    void do_i2f_u32_f64() override;
    void do_i2f_u64_f32() override;
    void do_i2f_u64_f64() override;

    void do_p2i_8() override;
    void do_p2i_16() override;
    void do_p2i_32() override;
    void do_p2i_64() override;

    void do_i2p_8() override;
    void do_i2p_16() override;
    void do_i2p_32() override;
    void do_i2p_64() override;

    void do_less_s8() override;
    void do_less_s16() override;
    void do_less_s32() override;
    void do_less_s64() override;
    void do_less_u8() override;
    void do_less_u16() override;
    void do_less_u32() override;
    void do_less_u64() override;
    void do_less_f32() override;
    void do_less_f64() override;
    void do_less_w32() override;
    void do_less_w64() override;

    void do_less_equal_s8() override;
    void do_less_equal_s16() override;
    void do_less_equal_s32() override;
    void do_less_equal_s64() override;
    void do_less_equal_u8() override;
    void do_less_equal_u16() override;
    void do_less_equal_u32() override;
    void do_less_equal_u64() override;
    void do_less_equal_f32() override;
    void do_less_equal_f64() override;
    void do_less_equal_w32() override;
    void do_less_equal_w64() override;

    void do_greater_s8() override;
    void do_greater_s16() override;
    void do_greater_s32() override;
    void do_greater_s64() override;
    void do_greater_u8() override;
    void do_greater_u16() override;
    void do_greater_u32() override;
    void do_greater_u64() override;
    void do_greater_f32() override;
    void do_greater_f64() override;
    void do_greater_w32() override;
    void do_greater_w64() override;

    void do_greater_equal_s8() override;
    void do_greater_equal_s16() override;
    void do_greater_equal_s32() override;
    void do_greater_equal_s64() override;
    void do_greater_equal_u8() override;
    void do_greater_equal_u16() override;
    void do_greater_equal_u32() override;
    void do_greater_equal_u64() override;
    void do_greater_equal_f32() override;
    void do_greater_equal_f64() override;
    void do_greater_equal_w32() override;
    void do_greater_equal_w64() override;

    void do_equal_u8() override;
    void do_equal_u16() override;
    void do_equal_u32() override;
    void do_equal_u64() override;
    void do_equal_f32() override;
    void do_equal_f64() override;
    void do_equal_w32() override;
    void do_equal_w64() override;

    void do_unequal_u8() override;
    void do_unequal_u16() override;
    void do_unequal_u32() override;
    void do_unequal_u64() override;
    void do_unequal_f32() override;
    void do_unequal_f64() override;
    void do_unequal_w32() override;
    void do_unequal_w64() override;

    void do_isnan_w32() override;
    void do_isnan_w64() override;

    void do_va_start() override;
    void do_va_end() override;
    void do_va_arg() override;
    void do_va_copy() override;

    void do_jump() override;
    void do_branch() override;
    void do_call() override;
    void do_ret() override;

    ExecState* state_;
    ExternCode* extern_code_;
    std::vector<Analyzer*> analyzers_;
    std::uint64_t  num_steps_;
};


}

#endif
