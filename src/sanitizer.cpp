#include <sala/sanitizer.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <algorithm>
#include <sstream>

namespace sala {


Sanitizer::Sanitizer(ExecState* const exec_state)
    : Analyzer{ exec_state }
    , regions_{}
    , changed_{ false }
{
    insert(&state().exit_code_memory_block());
    for (auto const& constant : state().constant_segment())
        insert(&constant);
    for (auto const& var : state().static_segment())
        insert(&var);
    for (auto const& ptr_and_block : state().heap_segment())
        insert(&ptr_and_block.second);
    Sanitizer::on_stack_initialized();
}


bool Sanitizer::inside(MemRegion const* const region, MemPtr const ptr, std::size_t const count) const
{
    return region != nullptr && region->first <= ptr && ptr + count <= region->first + region->second;
}


bool Sanitizer::is_memory_valid(MemPtr const ptr, std::size_t const count) const
{
if (!inside(locate(ptr), ptr, count))
{
    auto ppp = locate(ptr);
    int iii = 1;
}

    return inside(locate(ptr), ptr, count);
}


void Sanitizer::insert(MemPtr const ptr, std::size_t const count)
{
    regions_.push_back({ ptr, count });
    changed_ = true;
}


void Sanitizer::erase(MemPtr const ptr, std::size_t const count)
{
    auto const reg = locate(ptr);
    INVARIANT(reg != nullptr && reg->first == ptr && reg->second == count);
    std::swap(*reg, regions_.back());
    regions_.pop_back();
    changed_ = true;
}


void Sanitizer::insert(MemBlock const* const block)
{
    regions_.push_back({ block->start(), block->count() });
    changed_ = true;
}


void Sanitizer::erase(MemBlock const* block)
{
    auto const reg = locate(block->start());
    INVARIANT(reg != nullptr && block->start() == reg->first);
    std::swap(*reg, regions_.back());
    regions_.pop_back();
    changed_ = true;
}


Sanitizer::MemRegion* Sanitizer::locate(MemPtr const ptr) const
{
    struct local { static inline bool less(MemRegion const& a, MemRegion const& b) { return a.first < b.first; } };

    if (regions_.empty())
        return nullptr;

    if (changed_)
    {
        std::sort(regions_.begin(), regions_.end(), &local::less);
        changed_ = false;
    }

    auto const it = std::upper_bound(regions_.begin(), regions_.end(), MemRegion{ ptr, 0ULL }, &local::less);
    if (it == regions_.end())
        return &regions_.back();
    if (it == regions_.begin())
        return &regions_.front();
    return &*std::prev(it);
}


void Sanitizer::crash_interpretation(std::string const& text)
{
    state().set_stage(ExecState::Stage::FINISHED);
    state().set_termination(
        ExecState::Termination::CRASH,
        "test_interpreter[sanitizer]",
        state().make_error_message(text)
        );
    state().set_exit_code(0);
}


void Sanitizer::on_stack_initialized()
{
    for (auto const& record : state().stack_segment())
    {
        for (auto& param : record.parameters())
            insert(&param);
        for (auto& local : record.locals())
            insert(&local);
        for (auto& param : record.variadic_parameters())
            insert(&param);
    }
}


void Sanitizer::do_load()
{
    if (!is_memory_valid(operands().back()->as<MemPtr>(), operands().front()->count()))
        crash_interpretation_due_to_memory_access();
}


void Sanitizer::do_store()
{
    if (!is_memory_valid(operands().front()->as<MemPtr>(), operands().back()->count()))
        crash_interpretation_due_to_memory_access();
}


void Sanitizer::do_memcpy()
{
    auto const dst{ operands().front()->as<MemPtr>() };
    auto const src{ operands().at(1)->as<MemPtr>() };
    auto const size{ operands().back()->as_size() };

    if (!is_memory_valid(src, size) || !is_memory_valid(dst, size))
        crash_interpretation_due_to_memory_access();
    else if ((src >= dst && src < dst + size) || (dst >= src && dst < src + size))
        crash_interpretation("Memory blocks passed to memcpy overlap.");
}


void Sanitizer::do_memmove()
{
    if (!(is_memory_valid(operands().front()->as<MemPtr>(), operands().back()->as_size()) &&
                                   is_memory_valid(operands().at(1)->as<MemPtr>(), operands().back()->as_size())) )
        crash_interpretation_due_to_memory_access();
}


void Sanitizer::do_memset()
{
    if (!is_memory_valid(operands().front()->as<MemPtr>(), operands().back()->as_size()))
        crash_interpretation_due_to_memory_access();
}


void Sanitizer::do_alloca()
{
    set_post_operation([this]() {
        insert(&stack_top().locals().back());
    });
}


void Sanitizer::do_stackrestore()
{
    MemPtr const saved_top{ operands().front()->as<MemPtr>() };
    auto const reg = locate(saved_top);
    if (reg == nullptr || reg->first != saved_top)
    {
        crash_interpretation("Invalid stack restore pointer.");
        return;
    }

    for (auto rit = stack_top().locals().rbegin(), rend = stack_top().locals().rend(); rit != rend; ++rit)
        if (rit->start() == saved_top)
            return;

    crash_interpretation("Invalid stack restore pointer - cannot find restore variable.");
}


void Sanitizer::do_malloc()
{
    set_post_operation([this]() {
        auto const it = state().heap_segment().find(operands().front()->as<MemPtr>());
        if (it != state().heap_segment().end())
            insert(&it->second);
    });
}


void Sanitizer::do_free()
{
    MemPtr const ptr{ operands().front()->as<MemPtr>() };
    if (ptr == nullptr)
        return;

    auto const it = state().heap_segment().find(ptr);
    if (it != state().heap_segment().end())
    {
        erase(&it->second);
        return;
    }

    crash_interpretation("Cannot free memory from the heap since the passed pointer is not valid.");
}


void Sanitizer::do_div_s8()
{
    if (operands().at(2)->as<std::int8_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_s16()
{
    if (operands().at(2)->as<std::int16_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_s32()
{
    if (operands().at(2)->as<std::int32_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_s64()
{
    if (operands().at(2)->as<std::int64_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_u8()
{
    if (operands().at(2)->as<std::uint8_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_u16()
{
    if (operands().at(2)->as<std::uint16_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_u32()
{
    if (operands().at(2)->as<std::uint32_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_div_u64()
{
    if (operands().at(2)->as<std::uint64_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_s8()
{
    if (operands().at(2)->as<std::int8_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_s16()
{
    if (operands().at(2)->as<std::int16_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_s32()
{
    if (operands().at(2)->as<std::int32_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_s64()
{
    if (operands().at(2)->as<std::int64_t>() == 0)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_u8()
{
    if (operands().at(2)->as<std::uint8_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_u16()
{
    if (operands().at(2)->as<std::uint16_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_u32()
{
    if (operands().at(2)->as<std::uint32_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_rem_u64()
{
    if (operands().at(2)->as<std::uint64_t>() == 0U)
        crash_interpretation_due_to_zero_division();
}


void Sanitizer::do_call()
{
    if (instruction().descriptors().front() != Instruction::Descriptor::FUNCTION)
    {
        auto const it = state().functions_at_addresses().find(operands().front()->as<MemPtr>());
        if (it == state().functions_at_addresses().end())
        {
            crash_interpretation("Invalid function pointer.");
            return;
        }
        Function const& func = program().functions().at(it->second);

        if (operands().size() < func.parameters().size() + 1ULL)
        {
            crash_interpretation("Too few parameters for calling the function.");
            return;
        }

        for (std::size_t i = 0ULL; i != func.parameters().size(); ++i)
            if (operands().at(i + 1ULL)->count() != func.parameters().at(i).num_bytes())
            {
                std::stringstream sstr;
                sstr << "Parameter " << i << " expects " << func.parameters().at(i).num_bytes()
                        << " bytes, but the corresponding argument has " << operands().at(i + 1ULL)->count()
                        << " bytes."
                        ;
                crash_interpretation(sstr.str());
                return;
            }
    }

    set_post_operation([this]() {
        for (auto& param : state().stack_top().parameters())
            insert(&param);
        for (auto& local : state().stack_top().locals())
            insert(&local);
        for (auto& param : state().stack_top().variadic_parameters())
            insert(&param);
    });
}


void Sanitizer::do_ret()
{
    for (auto& param : state().stack_top().parameters())
        erase(&param);
    for (auto& local : state().stack_top().locals())
        erase(&local);
    for (auto& param : state().stack_top().variadic_parameters())
        erase(&param);
}


void Sanitizer::do_va_start()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
    if (!is_memory_valid((MemPtr)va_list_ptr, sizeof(*va_list_ptr)))
        crash_interpretation_due_to_memory_access();
    set_post_operation([this, va_list_ptr]() {
        insert((MemPtr)va_list_ptr->reg_save_area, (std::size_t)(va_list_ptr->gp_offset - 256U));
    });
}


void Sanitizer::do_va_end()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
    erase( (MemPtr)va_list_ptr->reg_save_area, (std::size_t)(va_list_ptr->gp_offset - 256U));
}


}
