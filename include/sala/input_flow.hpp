#ifndef SALA_INPUT_FLOW_HPP_INCLUDED
#   define SALA_INPUT_FLOW_HPP_INCLUDED

#   include <sala/analyzer.hpp>
#   include <vector>
#   include <unordered_set>
#   include <unordered_map>
#   include <memory>

namespace sala {


struct InputFlow : public Analyzer
{
    using InputDescriptor = std::uint32_t;

    struct FlowSet;
    using FlowSetPtr = std::shared_ptr<FlowSet>;

    struct FlowSet
    {
        static FlowSetPtr create();
        static FlowSetPtr create(InputDescriptor const& desc);
        FlowSet();
        explicit FlowSet(InputDescriptor desc);
        explicit FlowSet(FlowSet const& other);
        std::vector<InputDescriptor> const& descriptors() const { return descriptors_; }
        bool operator==(FlowSet const& other) const;
        bool operator!=(FlowSet const& other) const { return !(*this == other); }
        bool comprises(FlowSet const& addon) const;
        bool empty() const { return descriptors_.empty(); }
        void join(FlowSet const& addon);
    private:
        std::vector<InputDescriptor> descriptors_;
    };

    explicit InputFlow(ExecState* exec_state);

    void start(MemPtr ptr, InputDescriptor desc);
    void copy(MemPtr dst, MemPtr src, std::size_t count);
    void set(MemPtr dst, MemPtr ptr, std::size_t count);
    void move(MemPtr dst, MemPtr ptr, std::size_t count);
    void clear(MemPtr dst, std::size_t count);
    void join(MemPtr dst, std::size_t count, std::vector<std::pair<MemPtr, std::size_t> > const& memory);
    void join(MemPtr dst, MemPtr src, std::size_t count) { join(dst, count, { { src, count } }); }
    void join(MemPtr dst, MemPtr src1, MemPtr src2, std::size_t count) { join(dst, count, { { src1, count}, { src2, count} }); }
    void join(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count) { join(dst, dst_count, { { src, src_count} }); }
    void join(MemPtr dst, std::size_t dst_count, MemPtr src1, MemPtr src2, std::size_t src_count) { join(dst, dst_count, { { src1, src_count}, { src2, src_count} }); }
    void join_per_byte(MemPtr dst, MemPtr src1, MemPtr src2, std::size_t count);
    void extend_signed(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count);
    void extend_unsigned(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count);
    FlowSetPtr read(MemPtr ptr) const;

private:

    struct FlowSetHandle
    {
        FlowSetHandle() : pointer_{ nullptr}, hash_{ 0ULL} {}
        FlowSetHandle(FlowSetPtr flow);
        std::uint64_t hash() const { return hash_; }
        FlowSetPtr pointer() const { return pointer_; }
        bool operator==(FlowSetHandle const& other) const { return hash_ == other.hash_ && *pointer_ == *other.pointer_; }
        bool operator!=(FlowSetHandle const& other) const { return !(*this == other); }
        bool is_used() const { return pointer_.use_count() > 1; }
    private:
        FlowSetPtr pointer_;
        std::uint64_t hash_;
    };

    struct FlowSetHandleHasher { std::uint64_t operator()(FlowSetHandle const& handle) const { return handle.hash(); } };

    FlowSetHandle no_flow_;
    std::unordered_set<FlowSetHandle, FlowSetHandleHasher> handles_;
    std::unordered_map<MemPtr, FlowSetHandle> flow_;

    FlowSetHandle const& read_handle(MemPtr ptr) const;
    void write_handle(MemPtr ptr, FlowSetHandle const& handle);

    void register_external_functions();
    void register_external_llvm_intrinsics();
    void register_external_math_functions();
    void register_external_fenv_functions();

protected:

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

    void do_call() override;
    void do_ret() override;

    // External functions:

    virtual void pass_input_flow_from_parameters_to_return_value(std::size_t num_return_value_bytes);
    virtual void __llvm_intrinsic__bswap(std::size_t num_bytes);
    virtual void __llvm_intrinsic__ctlz(std::size_t num_bytes);
};


}

#endif
