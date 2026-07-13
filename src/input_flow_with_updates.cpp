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


InputFlowWithUpdates::InputFlowWithUpdates(ExecState* const exec_state)
    : InputFlow{ exec_state }
    , m_updates{}
{}


void InputFlowWithUpdates::start(MemPtr const ptr, InputDescriptor const desc)
{
    InputFlow::start(ptr, desc);
    write_updates(ptr, std::make_shared<UpdatesHitCounts>());
}


void InputFlowWithUpdates::start(MemPtr const ptr, std::size_t const count, InputDescriptor const desc)
{
    InputFlow::start(ptr, count, desc);
    UpdatesHitCountsPtr const updates{ std::make_shared<UpdatesHitCounts>() };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(ptr + i, updates);
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
    for (std::size_t i = 0ULL; i != count; ++i)
    {
        UpdatesHitCountsPtr const src_updates{ read_updates(src + i) };
        UpdatesHitCountsPtr dst_updates{ nullptr };
        if (src_updates != nullptr)
        {
            dst_updates = std::make_shared<UpdatesHitCounts>(*src_updates);
            (*dst_updates)[id] += 1U;
        }
        write_updates(dst + i, dst_updates);
    }
}


void InputFlowWithUpdates::set(MemPtr const dst, MemPtr const ptr, std::size_t const count)
{
    InputFlow::set(dst, ptr, count);
    UpdatesHitCountsPtr const updates{ read_updates(ptr) };
    for (std::size_t i = 0ULL; i != count; ++i)
        write_updates(dst + i, updates);
}


void InputFlowWithUpdates::move(MemPtr const dst, MemPtr const src, std::size_t const count)
{
    InputFlow::move(dst, src, count);
    std::vector<UpdatesHitCountsPtr> sources;
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
    UpdatesHitCountsPtr updates{ nullptr };
    for (auto const& ptr_and_count : memory)
        for (std::size_t i = 0ULL; i != ptr_and_count.second; ++i)
        {
            UpdatesHitCountsPtr const u{ read_updates(ptr_and_count.first + i) };
            if (u != nullptr)
            {
                if (updates == nullptr)
                    updates = std::make_shared<UpdatesHitCounts>(*u);
                else
                    merge_updates(*updates, *u);
            }
        }
    if (updates != nullptr)
    {
        InstructionID const id{
            .func = stack_top().function_index(),
            .block = ip().block(),
            .instr = ip().instr()
        };
        (*updates)[id] += 1U;
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
    UpdatesHitCountsPtr updates{ nullptr };
    for (std::size_t i = 0ULL; i != src_count; ++i)
    {
        UpdatesHitCountsPtr const u{ read_updates(src + i) };
        if (u != nullptr)
        {
            if (updates == nullptr)
                updates = std::make_shared<UpdatesHitCounts>(*u);
            else
                merge_updates(*updates, *u);
        }
    }
    if (updates == nullptr)
        return;
    InstructionID const id{
        .func = stack_top().function_index(),
        .block = ip().block(),
        .instr = ip().instr()
    };
    (*updates)[id] += 1U;
    for (std::size_t i = 0ULL; i != dst_count; ++i)
        merge_updates(dst + i, updates);
}


void InputFlowWithUpdates::extend_signed(MemPtr const dst, std::size_t const dst_count, MemPtr const src, std::size_t const src_count)
{
    InputFlow::extend_signed(dst, dst_count, src, src_count);
    for (std::size_t i = 0ULL; i != src_count; ++i)
         write_updates(dst + i, read_updates(src + i));
    UpdatesHitCountsPtr const ext = read_updates(src + (src_count - 1ULL));
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


InputFlowWithUpdates::UpdatesHitCountsPtr  InputFlowWithUpdates::read_updates(MemPtr const ptr) const
{
    auto const it = m_updates.find(ptr);
    return it == m_updates.end() ? nullptr : it->second;
}


void InputFlowWithUpdates::write_updates(MemPtr const ptr, UpdatesHitCountsPtr const updates)
{
    if (updates == nullptr)
        m_updates.erase(ptr);
    else
        m_updates[ptr] = updates;
}


void InputFlowWithUpdates::merge_updates(MemPtr const ptr, UpdatesHitCountsPtr const updates)
{
    if (updates != nullptr)
        merge_updates(*m_updates[ptr], *updates);
}


void InputFlowWithUpdates::merge_updates(UpdatesHitCounts& dest, UpdatesHitCounts const& updates)
{
    if (&dest != &updates)
        for (auto const& kv : updates)
            dest[kv.first] += kv.second;
}


}
