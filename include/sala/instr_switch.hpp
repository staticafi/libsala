#ifndef SALA_INSTR_SWITCH_HPP_INCLUDED
#   define SALA_INSTR_SWITCH_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/exec_state.hpp>
#   include <vector>

namespace sala {


struct InstrSwitch
{
    virtual ~InstrSwitch() {}

    virtual Instruction const& instruction() const = 0;
    virtual std::vector<MemBlock const*> const& operands() const = 0;

    // Return true iff one of the following methods were called:
    //      do_jump(), do_branch(), do_call(), do_ret()
    // Otherwise, returns false.
    bool do_instruction_switch();

    virtual void do_nop() {}

    virtual void do_halt() {}

    virtual void do_address() {}
    virtual void do_load() {}
    virtual void do_store() {}

    virtual void do_copy_8() {}
    virtual void do_copy_16() {}
    virtual void do_copy_32() {}
    virtual void do_copy_64() {}
    virtual void do_copy() {}

    virtual void do_memcpy() {}
    virtual void do_memmove() {}
    virtual void do_memset() {}
    virtual void do_moveptr() {}

    virtual void do_alloca() {}
    virtual void do_stacksave() {}
    virtual void do_stackrestore() {}
    virtual void do_malloc() {}
    virtual void do_free() {}

    virtual void do_add_s8() {}
    virtual void do_add_s16() {}
    virtual void do_add_s32() {}
    virtual void do_add_s64() {}
    virtual void do_add_u8() {}
    virtual void do_add_u16() {}
    virtual void do_add_u32() {}
    virtual void do_add_u64() {}
    virtual void do_add_f32() {}
    virtual void do_add_f64() {}

    virtual void do_sub_s8() {}
    virtual void do_sub_s16() {}
    virtual void do_sub_s32() {}
    virtual void do_sub_s64() {}
    virtual void do_sub_u8() {}
    virtual void do_sub_u16() {}
    virtual void do_sub_u32() {}
    virtual void do_sub_u64() {}
    virtual void do_sub_f32() {}
    virtual void do_sub_f64() {}

    virtual void do_mul_s8() {}
    virtual void do_mul_s16() {}
    virtual void do_mul_s32() {}
    virtual void do_mul_s64() {}
    virtual void do_mul_u8() {}
    virtual void do_mul_u16() {}
    virtual void do_mul_u32() {}
    virtual void do_mul_u64() {}
    virtual void do_mul_f32() {}
    virtual void do_mul_f64() {}

    virtual void do_div_s8() {}
    virtual void do_div_s16() {}
    virtual void do_div_s32() {}
    virtual void do_div_s64() {}
    virtual void do_div_u8() {}
    virtual void do_div_u16() {}
    virtual void do_div_u32() {}
    virtual void do_div_u64() {}
    virtual void do_div_f32() {}
    virtual void do_div_f64() {}

    virtual void do_rem_s8() {}
    virtual void do_rem_s16() {}
    virtual void do_rem_s32() {}
    virtual void do_rem_s64() {}
    virtual void do_rem_u8() {}
    virtual void do_rem_u16() {}
    virtual void do_rem_u32() {}
    virtual void do_rem_u64() {}

    virtual void do_and_8() {}
    virtual void do_and_16() {}
    virtual void do_and_32() {}
    virtual void do_and_64() {}

    virtual void do_or_8() {}
    virtual void do_or_16() {}
    virtual void do_or_32() {}
    virtual void do_or_64() {}

    virtual void do_xor_8() {}
    virtual void do_xor_16() {}
    virtual void do_xor_32() {}
    virtual void do_xor_64() {}

    virtual void do_shl_8() {}
    virtual void do_shl_16() {}
    virtual void do_shl_32() {}
    virtual void do_shl_64() {}

    virtual void do_shr_s8() {}
    virtual void do_shr_s16() {}
    virtual void do_shr_s32() {}
    virtual void do_shr_s64() {}
    virtual void do_shr_u8() {}
    virtual void do_shr_u16() {}
    virtual void do_shr_u32() {}
    virtual void do_shr_u64() {}

    virtual void do_neg_f32() {}
    virtual void do_neg_f64() {}

    virtual void do_extend_s8_s16() {}
    virtual void do_extend_s8_s32() {}
    virtual void do_extend_s8_s64() {}
    virtual void do_extend_s16_s32() {}
    virtual void do_extend_s16_s64() {}
    virtual void do_extend_s32_s64() {}
    virtual void do_extend_u8_u16() {}
    virtual void do_extend_u8_u32() {}
    virtual void do_extend_u8_u64() {}
    virtual void do_extend_u16_u32() {}
    virtual void do_extend_u16_u64() {}
    virtual void do_extend_u32_u64() {}
    virtual void do_extend_f32_f64() {}

    virtual void do_truncate_u64_u32() {}
    virtual void do_truncate_u64_u16() {}
    virtual void do_truncate_u64_u8() {}
    virtual void do_truncate_u32_u16() {}
    virtual void do_truncate_u32_u8() {}
    virtual void do_truncate_u16_u8() {}
    virtual void do_truncate_f64_f32() {}

    virtual void do_f2i_f32_s8() {}
    virtual void do_f2i_f32_s16() {}
    virtual void do_f2i_f32_s32() {}
    virtual void do_f2i_f32_s64() {}
    virtual void do_f2i_f32_u8() {}
    virtual void do_f2i_f32_u16() {}
    virtual void do_f2i_f32_u32() {}
    virtual void do_f2i_f32_u64() {}
    virtual void do_f2i_f64_s8() {}
    virtual void do_f2i_f64_s16() {}
    virtual void do_f2i_f64_s32() {}
    virtual void do_f2i_f64_s64() {}
    virtual void do_f2i_f64_u8() {}
    virtual void do_f2i_f64_u16() {}
    virtual void do_f2i_f64_u32() {}
    virtual void do_f2i_f64_u64() {}

    virtual void do_i2f_s8_f32() {}
    virtual void do_i2f_s8_f64() {}
    virtual void do_i2f_s16_f32() {}
    virtual void do_i2f_s16_f64() {}
    virtual void do_i2f_s32_f32() {}
    virtual void do_i2f_s32_f64() {}
    virtual void do_i2f_s64_f32() {}
    virtual void do_i2f_s64_f64() {}
    virtual void do_i2f_u8_f32() {}
    virtual void do_i2f_u8_f64() {}
    virtual void do_i2f_u16_f32() {}
    virtual void do_i2f_u16_f64() {}
    virtual void do_i2f_u32_f32() {}
    virtual void do_i2f_u32_f64() {}
    virtual void do_i2f_u64_f32() {}
    virtual void do_i2f_u64_f64() {}

    virtual void do_p2i_8() {}
    virtual void do_p2i_16() {}
    virtual void do_p2i_32() {}
    virtual void do_p2i_64() {}

    virtual void do_i2p_8() {}
    virtual void do_i2p_16() {}
    virtual void do_i2p_32() {}
    virtual void do_i2p_64() {}

    virtual void do_less_s8() {}
    virtual void do_less_s16() {}
    virtual void do_less_s32() {}
    virtual void do_less_s64() {}
    virtual void do_less_u8() {}
    virtual void do_less_u16() {}
    virtual void do_less_u32() {}
    virtual void do_less_u64() {}
    virtual void do_less_f32() {}
    virtual void do_less_f64() {}

    virtual void do_less_equal_s8() {}
    virtual void do_less_equal_s16() {}
    virtual void do_less_equal_s32() {}
    virtual void do_less_equal_s64() {}
    virtual void do_less_equal_u8() {}
    virtual void do_less_equal_u16() {}
    virtual void do_less_equal_u32() {}
    virtual void do_less_equal_u64() {}
    virtual void do_less_equal_f32() {}
    virtual void do_less_equal_f64() {}

    virtual void do_greater_s8() {}
    virtual void do_greater_s16() {}
    virtual void do_greater_s32() {}
    virtual void do_greater_s64() {}
    virtual void do_greater_u8() {}
    virtual void do_greater_u16() {}
    virtual void do_greater_u32() {}
    virtual void do_greater_u64() {}
    virtual void do_greater_f32() {}
    virtual void do_greater_f64() {}

    virtual void do_greater_equal_s8() {}
    virtual void do_greater_equal_s16() {}
    virtual void do_greater_equal_s32() {}
    virtual void do_greater_equal_s64() {}
    virtual void do_greater_equal_u8() {}
    virtual void do_greater_equal_u16() {}
    virtual void do_greater_equal_u32() {}
    virtual void do_greater_equal_u64() {}
    virtual void do_greater_equal_f32() {}
    virtual void do_greater_equal_f64() {}

    virtual void do_equal_u8() {}
    virtual void do_equal_u16() {}
    virtual void do_equal_u32() {}
    virtual void do_equal_u64() {}
    virtual void do_equal_f32() {}
    virtual void do_equal_f64() {}

    virtual void do_unequal_u8() {}
    virtual void do_unequal_u16() {}
    virtual void do_unequal_u32() {}
    virtual void do_unequal_u64() {}
    virtual void do_unequal_f32() {}
    virtual void do_unequal_f64() {}

    virtual void do_va_start() {}
    virtual void do_va_end() {}
    virtual void do_va_arg() {}

    virtual void do_jump() {}
    virtual void do_branch() {}
    virtual void do_call() {}
    virtual void do_ret() {}
};


}

#endif
