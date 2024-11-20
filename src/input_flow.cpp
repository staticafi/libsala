#include <sala/input_flow.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <sstream>
#include <cstring>
#include <cmath>
#include <cfenv>

namespace sala {


InputFlow::FlowSetPtr InputFlow::FlowSet::create()
{
    return std::make_shared<FlowSet>();
}


InputFlow::FlowSetPtr InputFlow::FlowSet::create(InputDescriptor const& desc)
{
    return std::make_shared<FlowSet>(desc);
}


InputFlow::FlowSet::FlowSet()
    : descriptors_{}
{}


InputFlow::FlowSet::FlowSet(InputDescriptor const desc)
    : descriptors_{ desc }
{}


InputFlow::FlowSet::FlowSet(FlowSet const& other)
    : descriptors_{ other.descriptors() }
{}


bool InputFlow::FlowSet::operator==(FlowSet const& other) const
{
    return this == &other || (descriptors().size() == other.descriptors().size() &&
                              std::equal(descriptors().begin(), descriptors().end(), other.descriptors().begin()));
}


bool InputFlow::FlowSet::comprises(FlowSet const& other) const
{
    if (this == &other)
        return true;
    if (descriptors().size() < other.descriptors().size())
        return false;
    std::unordered_set<InputDescriptor> const dict{ descriptors().begin(), descriptors().end() };
    for (auto desc : other.descriptors())
        if (!dict.contains(desc))
            return false;
    return true;
}


void InputFlow::FlowSet::join(FlowSet const& addon)
{
    if (this == &addon)
        return;
    std::unordered_set<InputDescriptor> result{ descriptors().begin(), descriptors().end() };
    result.insert(addon.descriptors().begin(), addon.descriptors().end());
    descriptors_.assign(result.begin(), result.end());
    std::sort(descriptors_.begin(), descriptors_.end());

    // The following commented code is faster version, but it does not discards duplicities:
    //      std::vector<InputDescriptor> result;
    //      std::merge(
    //          descriptors().begin(), descriptors().end(),
    //          addon.descriptors().begin(), addon.descriptors().end(),
    //          std::back_inserter(result)
    //          );
    //      std::swap(descriptors_, result);
}


InputFlow::FlowSetHandle::FlowSetHandle(FlowSetPtr const flow)
    : pointer_{ flow }
    , hash_{ 0ULL }
{
    for (InputDescriptor desc : pointer()->descriptors())
        ::hash_combine(hash_, desc);
}


InputFlow::InputFlow(ExecState* const exec_state)
    : Analyzer{ exec_state }
    , no_flow_{ FlowSet::create() }
    , handles_{ no_flow_ }
    , flow_{}
{
    register_external_functions();
}


void InputFlow::start(MemPtr const ptr, InputDescriptor const desc)
{
    write_handle(ptr, FlowSet::create(desc));
}


void InputFlow::copy(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    for (std::size_t i = 0ULL; i != count; ++i)
        write_handle(dst + i, read_handle(src + i));
}


void InputFlow::set(MemPtr const dst, MemPtr const ptr, std::size_t const count)
{
    FlowSetHandle const& handle{ read_handle(ptr) };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_handle(dst + i, handle);
}


void InputFlow::move(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    std::vector<FlowSetHandle> sources;
    for (std::size_t i = 0ULL; i != count; ++i)
        sources.push_back(read_handle(src + i));
    for (std::size_t i = 0ULL; i != count; ++i)
        write_handle(dst + i, sources.at(i));
}


void InputFlow::clear(MemPtr const dst, std::size_t const count)
{
    for (std::size_t i = 0ULL; i != count; ++i)
    {
        auto it = flow_.find(dst + i);
        if (it != flow_.end())
        {
            auto it_handle = handles_.find(it->second);
            flow_.erase(it);
            if (!it_handle->is_used())
                handles_.erase(it_handle);
        }
    }
}


void InputFlow::join(MemPtr const dst, std::size_t const count, std::vector<std::pair<MemPtr, std::size_t> > const& memory)
{
    FlowSetPtr flow{ FlowSet::create() };
    for (auto const& ptr_and_count : memory)
        for (std::size_t i = 0ULL; i != ptr_and_count.second; ++i)
            flow->join(*read(ptr_and_count.first + i));
    FlowSetHandle handle{ flow };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_handle(dst + i, handle);
}


void InputFlow::join_per_byte(MemPtr const dst, MemPtr const src1, MemPtr const src2, std::size_t const count)
{
    for (std::size_t i = 0ULL; i != count; ++i)
        join(dst + i, src1 + i, src2 + i, 1ULL);
    
}


void InputFlow::extend_signed(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    copy(dst, src, src_count);
    FlowSetHandle const& ext = read_handle(src + (src_count - 1ULL));
    for (std::size_t i = src_count; i < dst_count; ++i)
        write_handle(dst + i, ext);
}


void InputFlow::extend_unsigned(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    copy(dst, src, src_count);
    clear(dst + src_count, dst_count - src_count);
}


InputFlow::FlowSetPtr InputFlow::read(MemPtr ptr) const
{
    return read_handle(ptr).pointer();
}


InputFlow::FlowSetHandle const& InputFlow::read_handle(MemPtr const ptr) const
{
    auto const it = flow_.find(ptr);
    return it == flow_.end() ? no_flow_ : it->second;
}


void InputFlow::write_handle(MemPtr const ptr, FlowSetHandle const& handle)
{
    auto const it_and_state = handles_.insert(handle);
    flow_.insert_or_assign(ptr, it_and_state.second ? handle : *it_and_state.first);
}


void InputFlow::do_load()
{
    copy(operands().front()->start(), operands().back()->read<MemPtr>(), operands().front()->count());
}


void InputFlow::do_store()
{
    copy(operands().front()->read<MemPtr>(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_copy_8()
{
    copy(operands().front()->start(), operands().back()->start(), 1ULL);
}


void InputFlow::do_copy_16()
{
    copy(operands().front()->start(), operands().back()->start(), 2ULL);
}


void InputFlow::do_copy_32()
{
    copy(operands().front()->start(), operands().back()->start(), 4ULL);
}


void InputFlow::do_copy_64()
{
    copy(operands().front()->start(), operands().back()->start(), 8ULL);
}


void InputFlow::do_copy()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_memcpy()
{
    copy(operands().front()->read<MemPtr>(), operands().at(1)->read<MemPtr>(), operands().back()->as_size());
}


void InputFlow::do_memmove()
{
    move(operands().front()->read<MemPtr>(), operands().at(1)->read<MemPtr>(), operands().back()->as_size());
}


void InputFlow::do_memset()
{
    set(operands().front()->read<MemPtr>(), operands().at(1)->start(), operands().back()->as_size());
}


void InputFlow::do_moveptr()
{
    join(operands().front()->start(), operands().front()->count(),
        {
            { operands().at(1)->start(), operands().at(1)->count() },
            { operands().at(2)->start(), operands().at(2)->count() },
            { operands().at(3)->start(), operands().at(3)->count() }
        });
}


void InputFlow::do_free()
{
    MemPtr ptr{ operands().front()->read<MemPtr>() };
    clear(ptr, state().heap_segment().at(ptr).count());
}


void InputFlow::do_add_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_add_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_add_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_add_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_add_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_add_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_add_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_add_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_add_f32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_add_f64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_sub_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_sub_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_sub_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_sub_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_sub_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_sub_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_sub_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_sub_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_sub_f32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_sub_f64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_mul_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_mul_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_mul_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_mul_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_mul_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_mul_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_mul_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_mul_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_mul_f32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_mul_f64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_div_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_div_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_div_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_div_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_div_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_div_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_div_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_div_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_div_f32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_div_f64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_rem_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_rem_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_rem_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_rem_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_rem_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_rem_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_rem_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_rem_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_and_8()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_and_16()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_and_32()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_and_64()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_or_8()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_or_16()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_or_32()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_or_64()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_xor_8()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_xor_16()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_xor_32()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_xor_64()
{
    join_per_byte(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_shl_8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_shl_16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_shl_32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_shl_64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_shr_s8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_shr_s16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_shr_s32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_shr_s64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_shr_u8()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 1ULL);
}


void InputFlow::do_shr_u16()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 2ULL);
}


void InputFlow::do_shr_u32()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 4ULL);
}


void InputFlow::do_shr_u64()
{
    join(operands().front()->start(), operands().at(1)->start(), operands().at(2)->start(), 8ULL);
}


void InputFlow::do_neg_f32()
{
    join(operands().front()->start(), operands().back()->start(), 4ULL);
}


void InputFlow::do_neg_f64()
{
    join(operands().front()->start(), operands().back()->start(), 8ULL);
}


void InputFlow::do_extend_s8_s16()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_s8_s32()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_s8_s64()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_s16_s32()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_s16_s64()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_s32_s64()
{
    extend_signed(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u8_u16()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u8_u32()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u8_u64()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u16_u32()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u16_u64()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_u32_u64()
{
    extend_unsigned(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_extend_f32_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_truncate_u64_u32()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_u64_u16()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_u64_u8()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_u32_u16()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_u32_u8()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_u16_u8()
{
    copy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void InputFlow::do_truncate_f64_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f32_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_f2i_f64_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s8_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s8_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s16_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s16_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s32_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s32_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s64_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_s64_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u8_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u8_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u16_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u16_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u32_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u32_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u64_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2f_u64_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_p2i_8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_p2i_16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_p2i_32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_p2i_64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2p_8()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2p_16()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2p_32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_i2p_64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_less_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_less_equal_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_s8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_s16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_s32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_s64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_greater_equal_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_equal_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_u8()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_u16()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_u32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_u64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_f32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_f64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_unequal_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().at(1)->start(), operands().at(2)->start(), operands().back()->count());
}


void InputFlow::do_isnan_w32()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_isnan_w64()
{
    join(operands().front()->start(), operands().front()->count(), operands().back()->start(), operands().back()->count());
}


void InputFlow::do_va_start()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ (platform_linux_64_bit::va_list*)operands().front()->read<MemPtr>() };
    set_post_operation([this, va_list_ptr]() {
        MemPtr array = (MemPtr)va_list_ptr->reg_save_area;
        for (auto const& param : stack_top().variadic_parameters())
        {
            copy(array, (MemPtr)param.start(), param.count());
            std::size_t const param_size{ param.count() + (8ULL - param.count() % 8ULL) };
            array += param_size;
        }
    });
}


void InputFlow::do_va_end()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ (platform_linux_64_bit::va_list*)operands().front()->read<MemPtr>() };
    clear((MemPtr)va_list_ptr->reg_save_area, va_list_ptr->gp_offset - 256U);
}


void InputFlow::do_va_copy()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    NOT_IMPLEMENTED_YET();
}


void InputFlow::do_call()
{
    std::vector<MemBlock const*> const ops{ operands() };
    set_post_operation([this, ops]() {
        std::uint32_t idx = 1U;
        auto const& params = stack_top().parameters();
        for (std::uint32_t i = 0U; i < params.size(); ++i, ++idx)
            copy(params.at(i).start(), ops.at(idx)->start(), params.at(i).count());
        auto const& va_params = stack_top().variadic_parameters();
        for (std::uint32_t i = 0U; i < va_params.size(); ++i, ++idx)
            copy(va_params.at(i).start(), ops.at(idx)->start(), va_params.at(i).count());
    });
}


void InputFlow::do_ret()
{
    call_processor_of_current_function_if_registered_extern();

    for (auto const& block : stack_top().parameters())
        clear(block.start(), block.count());
    for (auto const& block : stack_top().locals())
        clear(block.start(), block.count());
    for (auto const& block : stack_top().variadic_parameters())
        clear(block.start(), block.count());
}


// External functions:


void InputFlow::register_external_functions()
{
    register_external_llvm_intrinsics();
    register_external_math_functions();
    register_external_string_functions();
    register_external_fenv_functions();
    register_external_linux_functions();
}


void InputFlow::register_external_llvm_intrinsics()
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__bswap_8, this->__llvm_intrinsic__bswap(1ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__bswap_16, this->__llvm_intrinsic__bswap(2ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__bswap_32, this->__llvm_intrinsic__bswap(4ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__bswap_64, this->__llvm_intrinsic__bswap(8ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctlz_8, this->__llvm_intrinsic__ctlz(1ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctlz_16, this->__llvm_intrinsic__ctlz(2ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctlz_32, this->__llvm_intrinsic__ctlz(4ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctlz_64, this->__llvm_intrinsic__ctlz(8ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctpop_8, this->__llvm_intrinsic__ctpop(1ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctpop_16, this->__llvm_intrinsic__ctpop(2ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctpop_32, this->__llvm_intrinsic__ctpop(4ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ctpop_64, this->__llvm_intrinsic__ctpop(8ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__trunc_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__trunc_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ceil_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ceil_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__floor_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__floor_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__round_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__round_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__rint_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__rint_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__abs_8, this->pass_input_flow_from_parameters_to_return_value(sizeof(std::int8_t)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__abs_16, this->pass_input_flow_from_parameters_to_return_value(sizeof(std::int16_t)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__abs_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(std::int32_t)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__abs_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(std::int64_t)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__maxnum_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__maxnum_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__minnum_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__minnum_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__copysign_32, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__copysign_64, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__is_fpclass_32, this->pass_input_flow_from_parameters_to_return_value(1ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__is_fpclass_64, this->pass_input_flow_from_parameters_to_return_value(1ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ptrmask_32, this->pass_input_flow_from_parameters_to_return_value(4ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ptrmask_64, this->pass_input_flow_from_parameters_to_return_value(8ULL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__sadd_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__sadd_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__sadd_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__uadd_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__uadd_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__uadd_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ssub_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ssub_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__ssub_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__usub_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__usub_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__usub_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__smul_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__smul_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__smul_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__umul_with_overflow_16(), this->pass_input_flow_from_parameters_to_return_value(2UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__umul_with_overflow_32(), this->pass_input_flow_from_parameters_to_return_value(4UL+1UL));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__llvm_intrinsic__umul_with_overflow_64(), this->pass_input_flow_from_parameters_to_return_value(8UL+1UL));
}


void InputFlow::register_external_math_functions()
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(acos, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(acosf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(acosh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(acoshf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(asin, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(asinf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(asinh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(asinhf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(atan, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(atanf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(atanh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(atanhf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(ceil, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(ceilf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(cos, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(cosf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(cosh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(coshf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(exp, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(expf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(exp2, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(exp2f, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fabs, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fabsf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(floor, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(floorf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(log, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(logf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(log2, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(log2f, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(log10, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(log10f, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(round, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(roundf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sin, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sinf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sinh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sinhf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sqrt, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(sqrtf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(tan, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(tanf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(tanh, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(tanhf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(trunc, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(truncf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );

    REGISTER_EXTERN_FUNCTION_PROCESSOR(__isinf, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__isnan, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__finite, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__signbit, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__fpclassify, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(__issignaling, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));

    REGISTER_EXTERN_FUNCTION_PROCESSOR(atan2, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(atan2f, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(copysign, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(copysignf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fmod, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fmodf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(remainder, this->pass_input_flow_from_parameters_to_return_value(sizeof(double)) );
    REGISTER_EXTERN_FUNCTION_PROCESSOR(remainderf, this->pass_input_flow_from_parameters_to_return_value(sizeof(float)) );

    REGISTER_EXTERN_FUNCTION_PROCESSOR(__iseqsig, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
}

void InputFlow::register_external_string_functions()
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strlen, this->strlen_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strchr, this->strchr_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strrchr, this->strrchr_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strspn, this->strspn_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strcspn, this->strcspn_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strpbrk, this->strpbrk_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strstr, this->strstr_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strtok, this->strtok_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strcat, this->strcat_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strncat, this->strncat_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strcpy, this->strcpy_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strncpy, this->strncpy_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strcmp, this->strcmp_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(strncmp, this->strncmp_impl());
}

void InputFlow::register_external_fenv_functions()
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fegetround, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
    REGISTER_EXTERN_FUNCTION_PROCESSOR(fesetround, this->pass_input_flow_from_parameters_to_return_value(sizeof(int)));
}

void InputFlow::register_external_linux_functions()
{
    REGISTER_EXTERN_FUNCTION_PROCESSOR(getopt, this->getopt_impl());
    REGISTER_EXTERN_FUNCTION_PROCESSOR(getopt_long, this->getopt_long_impl());
}

void InputFlow::pass_input_flow_from_parameters_to_return_value(std::size_t const num_return_value_bytes)
{
    std::vector<std::pair<MemPtr, std::size_t> > param_memory_regions;
    for (std::size_t i = 1ULL; i < parameters().size(); ++i)
        param_memory_regions.push_back({ parameters().at(i).start(), parameters().at(i).count() });
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    join(dst_ptr, num_return_value_bytes, param_memory_regions);
}


void InputFlow::__llvm_intrinsic__bswap(std::size_t const num_bytes)
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    auto const src_ptr{ parameters().back().start() };
    for (std::size_t i = 0ULL; i != num_bytes; ++i)
        copy(dst_ptr + num_bytes - (i + 1ULL), src_ptr + i, 1ULL);
}


void InputFlow::__llvm_intrinsic__ctlz(std::size_t const num_bytes)
{
    join(parameters().front().read<MemPtr>(), num_bytes, parameters().front().start(), num_bytes);
}


void InputFlow::__llvm_intrinsic__ctpop(std::size_t const num_bytes)
{
    join(parameters().front().read<MemPtr>(), num_bytes, parameters().front().start(), num_bytes);
}


void InputFlow::strlen_impl()
{
    // TODO!
}


void InputFlow::strchr_impl()
{
    // TODO!
}


void InputFlow::strrchr_impl()
{
    // TODO!
}


void InputFlow::strspn_impl()
{
    // TODO!
}


void InputFlow::strcspn_impl()
{
    // TODO!
}


void InputFlow::strpbrk_impl()
{
    // TODO!
}


void InputFlow::strstr_impl()
{
    // TODO!
}


void InputFlow::strtok_impl()
{
    // TODO!
}


void InputFlow::strcat_impl()
{
    // TODO!
}


void InputFlow::strncat_impl()
{
    // TODO!
}


void InputFlow::strcpy_impl()
{
    // TODO!
}


void InputFlow::strncpy_impl()
{
    // TODO!
}


void InputFlow::strcmp_impl()
{
    // TODO!
}


void InputFlow::strncmp_impl()
{
    // TODO!
}


void InputFlow::getopt_impl()
{
    // TODO!
}


void InputFlow::getopt_long_impl()
{
    // TODO!
}


}
