#include <sala/input_flow.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <algorithm>
#include <sstream>

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
    copy(operands().front()->start(), operands().back()->as<MemPtr>(), operands().front()->count());
}


void InputFlow::do_store()
{
    copy(operands().front()->as<MemPtr>(), operands().back()->start(), operands().back()->count());
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
    copy(operands().front()->as<MemPtr>(), operands().at(1)->as<MemPtr>(), operands().back()->as_size());
}


void InputFlow::do_memmove()
{
    move(operands().front()->as<MemPtr>(), operands().at(1)->as<MemPtr>(), operands().back()->as_size());
}


void InputFlow::do_memset()
{
    set(operands().front()->as<MemPtr>(), operands().at(1)->start(), operands().back()->as_size());
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
    MemPtr ptr{ operands().front()->as<MemPtr>() };
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


void InputFlow::do_va_start()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
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

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
    clear((MemPtr)va_list_ptr->reg_save_area, va_list_ptr->gp_offset - 256U);
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
    for (auto const& block : stack_top().parameters())
        clear(block.start(), block.count());
    for (auto const& block : stack_top().locals())
        clear(block.start(), block.count());
    for (auto const& block : stack_top().variadic_parameters())
        clear(block.start(), block.count());
}


}
