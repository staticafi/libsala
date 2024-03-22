#include <sala/interpreter.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>
#include <cstring>
#include <cmath>

namespace sala {


Interpreter::Interpreter(ExecState* const state, ExternCode* const extern_code, std::vector<Analyzer*> const& analyzers)
    : state_{ state }
    , extern_code_{ extern_code }
    , analyzers_{ analyzers }
{
    ASSUMPTION(program().num_cpu_bits() == 64U);
    // Support for 32-bit programs requires:
    //  (1) To introduce a translation between native 64-bit addresses and 32-bit
    //      handle identifying the addresses.
    //      There is a catch: How to recognize some handle is no longer valid (and
    //      so the corresponding mapping can be forgotten)?
    //  (2) Dedicated interpretation of instructions where the translation (1)
    //      is taken into account.
}


void Interpreter::step()
{
    if (done())
        return;

    if (instruction().opcode() == Instruction::Opcode::__INVALID__)
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("__INVALID__ instruction reached. ")
            );
        return;
    }

    for (auto& analyzer : analyzers())
    {
        analyzer->pre();
        if (done())
            return;
    }

    if (!do_instruction_switch())
        state().stack_top().ip().next();

    if (done())
        return;

    for (auto& analyzer : analyzers())
        analyzer->post();

    if (!done() && state().stack_segment().size() <= state().stack_exit_depth())
    {
        if (state().stage() == ExecState::Stage::INITIALIZING)
        {
            state().set_stage(ExecState::Stage::EXECUTING);

            state().stack_segment().push_back(StackRecord(program().functions().at(program().entry_function())));
            auto const& params{ state().stack_top().parameters() };

            // Currently we ignore parameters of the entry function (i.e., of "main").
            // So let's clear them, if there are any.
            for (auto const& param : params)
                std::memset(param.start(), 0, param.count());

            // The first parameter is actually the pointer to memory, where to write the return value.
            // So, let's point to the "exit_code" memory block.
            if (!params.empty() && params.front().count() == sizeof(MemPtr))
                params.front().as_ref<MemPtr>() = state().exit_code_memory_block().start();

            state().stack_top().ip().jump(0U);
        }
        else if (state().stage() != ExecState::Stage::FINISHED && !state().atexit_stack().empty())
        {
            state().set_stage(ExecState::Stage::TERMINATING);
            state().stack_segment().push_back(StackRecord(program().functions().at(state().pop_atexit_function())));
            state().stack_top().ip().jump(0U);
            state().update_current_values();
            extern_code_->execute();
        }
        else
        {
            state().set_stage(ExecState::Stage::FINISHED);
            state().set_termination(
                ExecState::Termination::NORMAL,
                "sala::Interpreter",
                "Returned from main()."
                );
            return;
        }

        state().update_current_values();

        for (auto& analyzer : analyzers())
            analyzer->on_stack_initialized();
    }
    else
        state().update_current_values();
}


void Interpreter::do_halt()
{
    state().set_stage(ExecState::Stage::FINISHED);
    state().set_termination(
        ExecState::Termination::ERROR,
        "sala::Interpreter",
        state().make_error_message("Instruction HALT.")
        );
}


void Interpreter::do_address()
{
    operands().front()->as_ref<MemPtr>() = operands().back()->start();
}


void Interpreter::do_load()
{
    std::memcpy(operands().front()->start(), operands().back()->as<MemPtr>(), operands().front()->count());
}


void Interpreter::do_store()
{
    std::memcpy(operands().front()->as<MemPtr>(), operands().back()->start(), operands().back()->count());
}


void Interpreter::do_copy_8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().back()->as<std::uint8_t>();
}


void Interpreter::do_copy_16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().back()->as<std::uint16_t>();
}


void Interpreter::do_copy_32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().back()->as<std::uint32_t>();
}


void Interpreter::do_copy_64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().back()->as<std::uint64_t>();
}


void Interpreter::do_copy()
{
    std::memcpy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void Interpreter::do_memcpy()
{
    std::memcpy(operands().front()->as<MemPtr>(), operands().at(1)->as<MemPtr>(), operands().back()->as_size());
}


void Interpreter::do_memmove()
{
    std::memmove(operands().front()->as<MemPtr>(), operands().at(1)->as<MemPtr>(), operands().back()->as_size());
}


void Interpreter::do_memset()
{
    std::memset(operands().front()->as<MemPtr>(), (int)operands().at(1)->as<std::uint8_t>(), operands().back()->as_size());
}


void Interpreter::do_moveptr()
{
    auto const shift = (std::int64_t)operands().at(2)->as_size() * (std::int64_t)operands().back()->as_size();
    operands().front()->as_ref<MemPtr>() = operands().at(1)->as<MemPtr>() + shift;
}


void Interpreter::do_alloca()
{
    state().stack_top().push_back_local_variable(operands().at(1)->as_size() * operands().at(2)->as_size());
    operands().front()->as_ref<MemPtr>() = state().stack_top().locals().back().start();
}


void Interpreter::do_stacksave()
{
    operands().front()->as_ref<MemPtr>() = state().stack_top().locals().back().start();
}


void Interpreter::do_stackrestore()
{
    MemPtr const saved_top{ operands().front()->as<MemPtr>() };
    while (state().stack_top().locals().back().start() != saved_top)
        state().stack_top().pop_back_local_variable();
}


void Interpreter::do_malloc()
{
    try
    {
        MemBlock mb{ operands().back()->as_size() };
        state().heap_segment().insert({ mb.start(), mb });
        operands().front()->as_ref<MemPtr>() = mb.start();
    }
    catch(const std::exception&)
    {
        operands().front()->as_ref<MemPtr>() = nullptr;
    }
}


void Interpreter::do_free()
{
    state().heap_segment().erase( operands().front()->as<MemPtr>() );
}


void Interpreter::do_add_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() + operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_add_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() + operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_add_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() + operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_add_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() + operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_add_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() + operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_add_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() + operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_add_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() + operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_add_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() + operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_add_f32()
{
    operands().front()->as_ref<float>() = operands().at(1)->as<float>() + operands().at(2)->as<float>();
}


void Interpreter::do_add_f64()
{
    operands().front()->as_ref<double>() = operands().at(1)->as<double>() + operands().at(2)->as<double>();
}


void Interpreter::do_sub_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() - operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_sub_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() - operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_sub_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() - operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_sub_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() - operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_sub_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() - operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_sub_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() - operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_sub_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() - operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_sub_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() - operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_sub_f32()
{
    operands().front()->as_ref<float>() = operands().at(1)->as<float>() - operands().at(2)->as<float>();
}


void Interpreter::do_sub_f64()
{
    operands().front()->as_ref<double>() = operands().at(1)->as<double>() - operands().at(2)->as<double>();
}


void Interpreter::do_mul_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() * operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_mul_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() * operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_mul_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() * operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_mul_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() * operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_mul_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() * operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_mul_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() * operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_mul_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() * operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_mul_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() * operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_mul_f32()
{
    operands().front()->as_ref<float>() = operands().at(1)->as<float>() * operands().at(2)->as<float>();
}


void Interpreter::do_mul_f64()
{
    operands().front()->as_ref<double>() = operands().at(1)->as<double>() * operands().at(2)->as<double>();
}


void Interpreter::do_div_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() / operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_div_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() / operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_div_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() / operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_div_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() / operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_div_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() / operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_div_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() / operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_div_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() / operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_div_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() / operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_div_f32()
{
    operands().front()->as_ref<float>() = operands().at(1)->as<float>() / operands().at(2)->as<float>();
}


void Interpreter::do_div_f64()
{
    operands().front()->as_ref<double>() = operands().at(1)->as<double>() / operands().at(2)->as<double>();
}


void Interpreter::do_rem_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() % operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_rem_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() % operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_rem_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() % operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_rem_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() % operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_rem_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() % operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_rem_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() % operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_rem_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() % operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_rem_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() % operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_and_8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() & operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_and_16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() & operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_and_32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() & operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_and_64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() & operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_or_8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() | operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_or_16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() | operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_or_32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() | operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_or_64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() | operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_xor_8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() ^ operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_xor_16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() ^ operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_xor_32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() ^ operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_xor_64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() ^ operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_shl_8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() << operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_shl_16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() << operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_shl_32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() << operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_shl_64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() << operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_shr_s8()
{
    operands().front()->as_ref<std::int8_t>() = operands().at(1)->as<std::int8_t>() >> operands().at(2)->as<std::int8_t>();
}


void Interpreter::do_shr_s16()
{
    operands().front()->as_ref<std::int16_t>() = operands().at(1)->as<std::int16_t>() >> operands().at(2)->as<std::int16_t>();
}


void Interpreter::do_shr_s32()
{
    operands().front()->as_ref<std::int32_t>() = operands().at(1)->as<std::int32_t>() >> operands().at(2)->as<std::int32_t>();
}


void Interpreter::do_shr_s64()
{
    operands().front()->as_ref<std::int64_t>() = operands().at(1)->as<std::int64_t>() >> operands().at(2)->as<std::int64_t>();
}


void Interpreter::do_shr_u8()
{
    operands().front()->as_ref<std::uint8_t>() = operands().at(1)->as<std::uint8_t>() >> operands().at(2)->as<std::uint8_t>();
}


void Interpreter::do_shr_u16()
{
    operands().front()->as_ref<std::uint16_t>() = operands().at(1)->as<std::uint16_t>() >> operands().at(2)->as<std::uint16_t>();
}


void Interpreter::do_shr_u32()
{
    operands().front()->as_ref<std::uint32_t>() = operands().at(1)->as<std::uint32_t>() >> operands().at(2)->as<std::uint32_t>();
}


void Interpreter::do_shr_u64()
{
    operands().front()->as_ref<std::uint64_t>() = operands().at(1)->as<std::uint64_t>() >> operands().at(2)->as<std::uint64_t>();
}


void Interpreter::do_neg_f32()
{
    operands().front()->as_ref<float>() = -operands().back()->as<float>();
}


void Interpreter::do_neg_f64()
{
    operands().front()->as_ref<double>() = -operands().back()->as<double>();
}


void Interpreter::do_extend_s8_s16()
{
    operands().front()->as_ref<std::int16_t>() = (std::int16_t)operands().back()->as<std::int8_t>();
}


void Interpreter::do_extend_s8_s32()
{
    operands().front()->as_ref<std::int32_t>() = (std::int32_t)operands().back()->as<std::int8_t>();
}


void Interpreter::do_extend_s8_s64()
{
    operands().front()->as_ref<std::int64_t>() = (std::int64_t)operands().back()->as<std::int8_t>();
}


void Interpreter::do_extend_s16_s32()
{
    operands().front()->as_ref<std::int32_t>() = (std::int32_t)operands().back()->as<std::int16_t>();
}


void Interpreter::do_extend_s16_s64()
{
    operands().front()->as_ref<std::int64_t>() = (std::int64_t)operands().back()->as<std::int16_t>();
}


void Interpreter::do_extend_s32_s64()
{
    operands().front()->as_ref<std::int64_t>() = (std::int64_t)operands().back()->as<std::int32_t>();
}


void Interpreter::do_extend_u8_u16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_extend_u8_u32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_extend_u8_u64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_extend_u16_u32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_extend_u16_u64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_extend_u32_u64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_extend_f32_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<float>();
}


void Interpreter::do_truncate_u64_u32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_truncate_u64_u16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_truncate_u64_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_truncate_u32_u16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_truncate_u32_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_truncate_u16_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_truncate_f64_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<double>();
}


void Interpreter::do_f2i_f32_s8()
{
    operands().front()->as_ref<std::int8_t>() = (std::int8_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_s16()
{
    operands().front()->as_ref<std::int16_t>() = (std::int16_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_s32()
{
    operands().front()->as_ref<std::int32_t>() = (std::int32_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_s64()
{
    operands().front()->as_ref<std::int64_t>() = (std::int64_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_u16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_u32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f32_u64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<float>();
}


void Interpreter::do_f2i_f64_s8()
{
    operands().front()->as_ref<std::int8_t>() = (std::int8_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_s16()
{
    operands().front()->as_ref<std::int16_t>() = (std::int16_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_s32()
{
    operands().front()->as_ref<std::int32_t>() = (std::int32_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_s64()
{
    operands().front()->as_ref<std::int64_t>() = (std::int64_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_u16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_u32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)operands().back()->as<double>();
}


void Interpreter::do_f2i_f64_u64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<double>();
}


void Interpreter::do_i2f_s8_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::int8_t>();
}


void Interpreter::do_i2f_s8_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::int8_t>();
}


void Interpreter::do_i2f_s16_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::int16_t>();
}


void Interpreter::do_i2f_s16_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::int16_t>();
}


void Interpreter::do_i2f_s32_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::int32_t>();
}


void Interpreter::do_i2f_s32_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::int32_t>();
}


void Interpreter::do_i2f_s64_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::int64_t>();
}


void Interpreter::do_i2f_s64_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::int64_t>();
}


void Interpreter::do_i2f_u8_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_i2f_u8_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_i2f_u16_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_i2f_u16_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_i2f_u32_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_i2f_u32_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_i2f_u64_f32()
{
    operands().front()->as_ref<float>() = (float)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_i2f_u64_f64()
{
    operands().front()->as_ref<double>() = (double)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_p2i_8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(std::size_t)operands().back()->as<MemPtr>();
}


void Interpreter::do_p2i_16()
{
    operands().front()->as_ref<std::uint16_t>() = (std::uint16_t)(std::size_t)operands().back()->as<MemPtr>();
}


void Interpreter::do_p2i_32()
{
    operands().front()->as_ref<std::uint32_t>() = (std::uint32_t)(std::size_t)operands().back()->as<MemPtr>();
}


void Interpreter::do_p2i_64()
{
    operands().front()->as_ref<std::uint64_t>() = (std::uint64_t)operands().back()->as<MemPtr>();
}


void Interpreter::do_i2p_8()
{
    operands().front()->as_ref<MemPtr>() = (MemPtr)(std::size_t)operands().back()->as<std::uint8_t>();
}


void Interpreter::do_i2p_16()
{
    operands().front()->as_ref<MemPtr>() = (MemPtr)(std::size_t)operands().back()->as<std::uint16_t>();
}


void Interpreter::do_i2p_32()
{
    operands().front()->as_ref<MemPtr>() = (MemPtr)(std::size_t)operands().back()->as<std::uint32_t>();
}


void Interpreter::do_i2p_64()
{
    operands().front()->as_ref<MemPtr>() = (MemPtr)operands().back()->as<std::uint64_t>();
}


void Interpreter::do_less_s8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int8_t>() < operands().at(2)->as<std::int8_t>());
}


void Interpreter::do_less_s16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int16_t>() < operands().at(2)->as<std::int16_t>());
}


void Interpreter::do_less_s32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int32_t>() < operands().at(2)->as<std::int32_t>());
}


void Interpreter::do_less_s64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int64_t>() < operands().at(2)->as<std::int64_t>());
}


void Interpreter::do_less_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() < operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_less_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() < operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_less_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() < operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_less_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() < operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_less_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() < operands().at(2)->as<float>());
}


void Interpreter::do_less_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() < operands().at(2)->as<double>());
}


void Interpreter::do_less_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f < g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_less_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f < g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_less_equal_s8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int8_t>() <= operands().at(2)->as<std::int8_t>());
}


void Interpreter::do_less_equal_s16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int16_t>() <= operands().at(2)->as<std::int16_t>());
}


void Interpreter::do_less_equal_s32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int32_t>() <= operands().at(2)->as<std::int32_t>());
}


void Interpreter::do_less_equal_s64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int64_t>() <= operands().at(2)->as<std::int64_t>());
}


void Interpreter::do_less_equal_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() <= operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_less_equal_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() <= operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_less_equal_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() <= operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_less_equal_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() <= operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_less_equal_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() <= operands().at(2)->as<float>());
}


void Interpreter::do_less_equal_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() <= operands().at(2)->as<double>());
}


void Interpreter::do_less_equal_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f <= g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_less_equal_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f <= g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_greater_s8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int8_t>() > operands().at(2)->as<std::int8_t>());
}


void Interpreter::do_greater_s16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int16_t>() > operands().at(2)->as<std::int16_t>());
}


void Interpreter::do_greater_s32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int32_t>() > operands().at(2)->as<std::int32_t>());
}


void Interpreter::do_greater_s64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int64_t>() > operands().at(2)->as<std::int64_t>());
}


void Interpreter::do_greater_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() > operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_greater_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() > operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_greater_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() > operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_greater_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() > operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_greater_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() > operands().at(2)->as<float>());
}


void Interpreter::do_greater_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() > operands().at(2)->as<double>());
}


void Interpreter::do_greater_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f > g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_greater_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f > g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_greater_equal_s8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int8_t>() >= operands().at(2)->as<std::int8_t>());
}


void Interpreter::do_greater_equal_s16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int16_t>() >= operands().at(2)->as<std::int16_t>());
}


void Interpreter::do_greater_equal_s32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int32_t>() >= operands().at(2)->as<std::int32_t>());
}


void Interpreter::do_greater_equal_s64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::int64_t>() >= operands().at(2)->as<std::int64_t>());
}


void Interpreter::do_greater_equal_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() >= operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_greater_equal_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() >= operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_greater_equal_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() >= operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_greater_equal_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() >= operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_greater_equal_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() >= operands().at(2)->as<float>());
}


void Interpreter::do_greater_equal_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() >= operands().at(2)->as<double>());
}


void Interpreter::do_greater_equal_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f >= g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_greater_equal_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f >= g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_equal_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() == operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_equal_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() == operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_equal_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() == operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_equal_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() == operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_equal_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() == operands().at(2)->as<float>());
}


void Interpreter::do_equal_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() == operands().at(2)->as<double>());
}


void Interpreter::do_equal_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f == g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_equal_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f == g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_unequal_u8()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint8_t>() != operands().at(2)->as<std::uint8_t>());
}


void Interpreter::do_unequal_u16()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint16_t>() != operands().at(2)->as<std::uint16_t>());
}


void Interpreter::do_unequal_u32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint32_t>() != operands().at(2)->as<std::uint32_t>());
}


void Interpreter::do_unequal_u64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<std::uint64_t>() != operands().at(2)->as<std::uint64_t>());
}


void Interpreter::do_unequal_f32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<float>() != operands().at(2)->as<float>());
}


void Interpreter::do_unequal_f64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)(operands().at(1)->as<double>() != operands().at(2)->as<double>());
}


void Interpreter::do_unequal_w32()
{
    auto const f{ operands().at(1)->as<float>() };
    auto const g{ operands().at(2)->as<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f != g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_unequal_w64()
{
    auto const f{ operands().at(1)->as<double>() };
    auto const g{ operands().at(2)->as<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f != g };
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)cond;
}


void Interpreter::do_isnan_w32()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)std::isnan(operands().back()->as<float>());
}


void Interpreter::do_isnan_w64()
{
    operands().front()->as_ref<std::uint8_t>() = (std::uint8_t)std::isnan(operands().back()->as<double>());
}


void Interpreter::do_va_start()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    std::size_t array_size{ 0ULL };
    for (auto const& param : state().stack_top().variadic_parameters())
    {
        std::size_t const param_size{ param.count() + (8ULL - param.count() % 8ULL) };
        array_size += param_size;
    }
    if (array_size + 256U > std::numeric_limits<std::uint32_t>::max())
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("Cannot allocate memory for variadic parameters. The size must fit into 32-bit unsigned integer.")
            );
        return;
    }

    MemPtr array;
    try
    {
        MemBlock mb{ array_size };
        array = state().heap_segment().insert({ mb.start(), mb }).first->first;
    }
    catch(const std::exception&)
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("Cannot allocate memory for variadic parameters.")
            );
        return;
    }

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
    va_list_ptr->gp_offset = 256U + (std::uint32_t)array_size;
    va_list_ptr->fp_offset = 256U;
    va_list_ptr->overflow_arg_area = array;
    va_list_ptr->reg_save_area = array;

    for (auto const& param : state().stack_top().variadic_parameters())
    {
        std::memcpy((void*)array, (void*)param.start(), param.count());
        std::size_t const param_size{ param.count() + (8ULL - param.count() % 8ULL) };
        array += param_size;
    }
}


void Interpreter::do_va_end()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    platform_linux_64_bit::va_list* const va_list_ptr{ operands().front()->as_ref<platform_linux_64_bit::va_list*>() };
    state().heap_segment().erase( (MemPtr)va_list_ptr->reg_save_area );
}


void Interpreter::do_va_arg()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // There is nothing to do. According to inspection of code produced by Clang the effect
    // of the 'va_arg' macro is already encoded in the body of the variadic function.
}


void Interpreter::do_jump()
{
    ip().jump( block().successors().front() );
}


void Interpreter::do_branch()
{
    ip().jump(
        *(std::uint8_t*)operands().front()->start() == 0U ?
            block().successors().front() :
            block().successors().back()
        );
}


void Interpreter::do_call()
{
    std::uint32_t func_idx;
    if (instruction().descriptors().front() == Instruction::Descriptor::FUNCTION)
        func_idx = instruction().operands().front();
    else
        func_idx = state().functions_at_addresses().at(operands().front()->as<MemPtr>());

    state().stack_top().ip().next();

    Function const& func = program().functions().at(func_idx);

    state().stack_segment().push_back(StackRecord(func));

    auto const& params = state().stack_top().parameters();

    std::uint32_t idx = 0U;
    for ( ; (std::size_t)idx < func.parameters().size(); ++idx)
        std::memcpy(params.at(idx).start(), operands().at(1U + idx)->start(), params.at(idx).count());
    for ( ; (std::size_t)(1U + idx) < operands().size(); ++idx)
    {
        auto const op = operands().at(1U + idx);
        state().stack_top().push_back_variadic_parameter(op->count());
        std::memcpy(state().stack_top().variadic_parameters().back().start(), op->start(), op->count());
    }

    state().update_current_values();

    extern_code_->execute();
}


void Interpreter::do_ret()
{
    state().stack_segment().pop_back();
}


}
