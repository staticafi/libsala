#include <sala/input_flow_with_updates.hpp>
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
#include <unistd.h>
#include <getopt.h>

namespace sala {


InputFlowWithUpdates::InputFlowWithUpdates(ExecState* const exec_state, bool const is_predicate_update)
    : InputFlow{ exec_state }
    , m_updates{}
    , m_is_predicate_update{ is_predicate_update }
    , m_basic_block_counter{ 0U }
{}


void InputFlowWithUpdates::start(MemPtr const ptr, InputDescriptor const desc)
{
    InputFlow::start(ptr, desc);
    write_updates(ptr, std::make_shared<UpdatesHits>());
}


void InputFlowWithUpdates::start(MemPtr const ptr, std::size_t const count, InputDescriptor const desc)
{
    InputFlow::start(ptr, count, desc);
    UpdatesHitsPtr const updates{ std::make_shared<UpdatesHits>() };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(ptr + i, updates);
}


void InputFlowWithUpdates::start_extend(MemPtr const ptr, InputDescriptor const desc)
{
    InputFlow::start_extend(ptr, desc);
    merge_updates(ptr, std::make_shared<UpdatesHits>());

}


void InputFlowWithUpdates::start_extend(MemPtr const ptr, std::size_t const count, InputDescriptor const desc)
{
    InputFlow::start_extend(ptr, count, desc);
    UpdatesHitsPtr const updates{ std::make_shared<UpdatesHits>() };
    for (std::size_t i = 0ULL; i != count; ++i)
        merge_updates(ptr + i, updates);
}


void InputFlowWithUpdates::copy(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    InputFlow::copy(dst, src, count);
    for (std::size_t i = 0ULL; i != count; ++i)
         write_updates(dst + i, read_updates(src + i));
}


void InputFlowWithUpdates::slice(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    InputFlow::slice(dst, src, count);
    InstructionID const id{
        .func = stack_top().function_index(),
        .block = ip().block(),
        .instr = ip().instr()
    };
    bool const is_update{ is_current_instruction_update() };
    for (std::size_t i = 0ULL; i != count; ++i)
    {
        UpdatesHitsPtr const src_updates{ read_updates(src + i) };
        if (is_update)
        {
            UpdatesHitsPtr dst_updates{ nullptr };
            if (src_updates != nullptr)
            {
                dst_updates = std::make_shared<UpdatesHits>(*src_updates);
                (*dst_updates)[id].insert(m_basic_block_counter);
            }
            write_updates(dst + i, dst_updates);
        }
        else
            write_updates(dst + i, src_updates);
    }
}


void InputFlowWithUpdates::set(MemPtr const dst, MemPtr const ptr, std::size_t const count)
{
    InputFlow::set(dst, ptr, count);
    UpdatesHitsPtr const updates{ read_updates(ptr) };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(dst + i, updates);
}


void InputFlowWithUpdates::move(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    InputFlow::move(dst, src, count);
    std::vector<UpdatesHitsPtr> sources;
    for (std::size_t i = 0ULL; i != count; ++i)
        sources.push_back(read_updates(src + i));
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(dst + i, sources.at(i));
}


void InputFlowWithUpdates::clear(MemPtr const dst, std::size_t const count)
{
    InputFlow::clear(dst, count);
    for (std::size_t i = 0ULL; i != count; ++i)
        m_updates.erase(dst + i);
}


void InputFlowWithUpdates::join(MemPtr const dst, std::size_t const count, std::vector<std::pair<MemPtr, std::size_t> > const& memory)
{
    InputFlow::join(dst, count, memory);
    UpdatesHitsPtr updates{ nullptr };
    for (auto const& ptr_and_count : memory)
        for (std::size_t i = 0ULL; i != ptr_and_count.second; ++i)
        {
            UpdatesHitsPtr const u{ read_updates(ptr_and_count.first + i) };
            if (u != nullptr)
            {
                if (updates == nullptr)
                    updates = std::make_shared<UpdatesHits>(*u);
                else
                    merge_updates(*updates, *u);
            }
        }
    if (updates != nullptr && is_current_instruction_update())
    {
        InstructionID const id{
            .func = stack_top().function_index(),
            .block = ip().block(),
            .instr = ip().instr()
        };
        (*updates)[id].insert(m_basic_block_counter);
    }
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(dst + i, updates);
}


void InputFlowWithUpdates::join_per_byte(MemPtr const dst, MemPtr const src1, MemPtr const src2, std::size_t const count)
{
    // We do NOT call 'InputFlow::join_per_byte' because we call 'join' (which calls 'InputFlow::join').
    for (std::size_t i = 0ULL; i != count; ++i)
        join(dst + i, 1ULL, { { src1 + i, 1ULL }, { src2 + i, 1ULL } });
}


void InputFlowWithUpdates::join_extend(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    InputFlow::join_extend(dst, dst_count, src, src_count);
    UpdatesHitsPtr updates{ nullptr };
    for (std::size_t i = 0ULL; i != src_count; ++i)
    {
        UpdatesHitsPtr const u{ read_updates(src + i) };
        if (u != nullptr)
        {
            if (updates == nullptr)
                updates = std::make_shared<UpdatesHits>(*u);
            else
                merge_updates(*updates, *u);
        }
    }
    if (updates == nullptr)
        return;
    if (is_current_instruction_update())
    {
        InstructionID const id{
            .func = stack_top().function_index(),
            .block = ip().block(),
            .instr = ip().instr()
        };
        (*updates)[id].insert(m_basic_block_counter);
    }
    for (std::size_t i = 0ULL; i != dst_count; ++i)
        merge_updates(dst + i, updates);
}


void InputFlowWithUpdates::extend_signed(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    InputFlow::extend_signed(dst, dst_count, src, src_count);
    for (std::size_t i = 0ULL; i != src_count; ++i)
         write_updates(dst + i, read_updates(src + i));
    UpdatesHitsPtr const ext = read_updates(src + (src_count - 1ULL));
    for (std::size_t i = src_count; i < dst_count; ++i)
        write_updates(dst + i, ext);
}


void InputFlowWithUpdates::extend_unsigned(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    InputFlow::extend_unsigned(dst, dst_count, src, src_count);
    for (std::size_t i = 0ULL; i != src_count; ++i)
         write_updates(dst + i, read_updates(src + i));
    for (std::size_t i = 0ULL; i != dst_count - src_count; ++i)
        m_updates.erase(dst + src_count + i);
}


InputFlowWithUpdates::UpdatesHitsPtr  InputFlowWithUpdates::read_updates(MemPtr const ptr) const
{
    auto const it = m_updates.find(ptr);
    return it == m_updates.end() ? nullptr : it->second;
}


void InputFlowWithUpdates::write_updates(MemPtr const ptr, UpdatesHitsPtr const updates)
{
    if (updates == nullptr)
        m_updates.erase(ptr);
    else
        m_updates[ptr] = updates;
}


void InputFlowWithUpdates::merge_updates(MemPtr const ptr, UpdatesHitsPtr const updates)
{
    if (updates != nullptr)
        merge_updates(*m_updates[ptr], *updates);
}


void InputFlowWithUpdates::merge_updates(UpdatesHits& dest, UpdatesHits const& updates)
{
    if (&dest != &updates)
        for (auto const& kv : updates)
            dest[kv.first].insert(kv.second.begin(), kv.second.end());
}


bool InputFlowWithUpdates::is_current_instruction_update() const
{
    if (!m_is_predicate_update)
        switch (state().current_instruction().opcode())
        {
            case Instruction::Opcode::LESS:
            case Instruction::Opcode::LESS_EQUAL:
            case Instruction::Opcode::GREATER:
            case Instruction::Opcode::GREATER_EQUAL:
            case Instruction::Opcode::EQUAL:
            case Instruction::Opcode::UNEQUAL:
            case Instruction::Opcode::ISNAN:
                return false;
            default: break;
        }
    return true;
}


void InputFlowWithUpdates::on_basic_block_changed()
{
    ++m_basic_block_counter;
}


void InputFlowWithUpdates::do_jump()
{
    InputFlow::do_jump();
    on_basic_block_changed();
}


void InputFlowWithUpdates::do_branch()
{
    InputFlow::do_branch();
    on_basic_block_changed();
}


void InputFlowWithUpdates::do_call()
{
    InputFlow::do_call();
    on_basic_block_changed();
}


void InputFlowWithUpdates::do_ret()
{
    InputFlow::do_ret();
    on_basic_block_changed();
}


}
