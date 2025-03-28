#ifndef SALA_SANITIZER_HPP_INCLUDED
#   define SALA_SANITIZER_HPP_INCLUDED

#   include <sala/analyzer.hpp>
#   include <map>

namespace sala {


struct Sanitizer : public Analyzer
{
    using MemRegionsMap = std::map<MemPtr, std::size_t>;
    using MemRegion = MemRegionsMap::value_type;

    explicit Sanitizer(ExecState* exec_state);

    bool inside(MemRegion const* region, MemPtr ptr, std::size_t count) const;
    bool is_memory_valid(MemPtr ptr, std::size_t count) const;
    bool is_c_string_valid(MemPtr str) const;
    bool is_c_string_valid(MemPtr str, std::size_t max_len) const;

private:
    mutable MemRegionsMap regions_;

    void insert(MemPtr ptr, std::size_t count);
    void erase(MemPtr const ptr, std::size_t count);

    void insert(MemBlock const* block);
    void erase(MemBlock const* block);

    MemRegion* locate(MemPtr ptr) const;
    MemRegionsMap::iterator find(MemPtr ptr) const;

    void crash_interpretation(std::string const& text);
    void crash_interpretation_due_to_memory_access();
    void crash_interpretation_due_to_zero_division();

    void on_stack_initialized() override;

    void do_load() override;
    void do_store() override;
    void do_memcpy() override;
    void do_memmove() override;
    void do_memset() override;
    void do_alloca() override;
    void do_stackrestore() override;
    void do_malloc() override;
    void do_free() override;
    void do_div_s8() override;
    void do_div_s16() override;
    void do_div_s32() override;
    void do_div_s64() override;
    void do_div_u8() override;
    void do_div_u16() override;
    void do_div_u32() override;
    void do_div_u64() override;
    void do_rem_s8() override;
    void do_rem_s16() override;
    void do_rem_s32() override;
    void do_rem_s64() override;
    void do_rem_u8() override;
    void do_rem_u16() override;
    void do_rem_u32() override;
    void do_rem_u64() override;
    void do_call() override;
    void do_ret() override;
    void do_va_start() override;
    void do_va_end() override;
    void do_va_copy() override;
};


}

#endif
