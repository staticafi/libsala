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
    , num_steps_{ 0ULL }
{}


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
    ++num_steps_;

    if (done())
        return;

    for (auto& analyzer : analyzers())
        analyzer->post();

    if (!done() && state().stack_segment().size() <= state().stack_exit_depth())
    {
        if (state().stage() == ExecState::Stage::INITIALIZING)
        {
            state().set_stage(ExecState::Stage::EXECUTING);

            state().stack_segment().push_back(StackRecord(state().pointer_model(), program().functions().at(program().entry_function())));
            auto const& params{ state().stack_top().parameters() };

            if (params.empty())
            {
                // void main(void)
                // => nothing to do.
            }
            else if (params.size() == 1ULL)
            {
                // int main(void)
                ASSUMPTION(params.front().count() == state().pointer_model()->sizeof_pointer());
                params.front().write(state().exit_code_memory_block().start());
            }
            else if (params.size() == 2ULL)
            {
                // void main(int argc, char* argv[])
                ASSUMPTION(params.front().count() == sizeof(int));
                ASSUMPTION(params.back().count() == state().pointer_model()->sizeof_pointer());
                params.front().write(state().argc());
                params.back().write(state().argv().start());
            }
            else
            {
                // int main(int argc, char* argv[])
                ASSUMPTION(params.size() == 3ULL);
                ASSUMPTION(params.front().count() == state().pointer_model()->sizeof_pointer());
                ASSUMPTION(params.at(1ULL).count() == sizeof(int));
                ASSUMPTION(params.back().count() == state().pointer_model()->sizeof_pointer());
                params.front().write(state().exit_code_memory_block().start());
                params.at(1ULL).write(state().argc());
                params.back().write(state().argv().start());
            }

            state().stack_top().ip().jump(0U);
        }
        else if (state().stage() != ExecState::Stage::FINISHED && !state().atexit_stack().empty())
        {
            state().set_stage(ExecState::Stage::TERMINATING);
            state().stack_segment().push_back(StackRecord(state().pointer_model(), program().functions().at(state().pop_atexit_function())));
            state().stack_top().ip().jump(0U);
            state().update_current_values();
            extern_code_->call_code_of_current_function_if_registered_external();
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


void Interpreter::run()
{
    while (!done())
        step();
}


void Interpreter::run(double const max_seconds)
{
    std::chrono::system_clock::time_point const  start_time = std::chrono::system_clock::now();
    run([start_time, max_seconds](std::string& error_message) {
        double const num_seconds = std::chrono::duration<double>(std::chrono::system_clock::now() - start_time).count();
        if (num_seconds >= max_seconds)
        {
            error_message = "[TIME OUT] The time budget " + std::to_string(max_seconds) + "s for the execution was exhausted.";
            return true;
        }
        return false;
    });
}


void Interpreter::run(std::function<bool(std::string&)> const&  terminator)
{
    std::string  error_message;
    while (!done())
    {
        error_message.clear();
        if (terminator(error_message))
        {
            state().set_stage(ExecState::Stage::FINISHED);
            state().set_termination(
                ExecState::Termination::ERROR,
                "sala::Interpreter",
                state().make_error_message(error_message + " [Processed instructions: " + std::to_string(num_steps()) + "]")
                );
            return;
        }

        step();
    }
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
    operands().front()->write<MemPtr>(operands().back()->start());
}


void Interpreter::do_load()
{
    std::memcpy(operands().front()->start(), operands().back()->read<MemPtr>(), operands().front()->count());
}


void Interpreter::do_store()
{
    std::memcpy(operands().front()->read<MemPtr>(), operands().back()->start(), operands().back()->count());
}


void Interpreter::do_copy_8()
{
    operands().front()->write<std::uint8_t>(operands().back()->read<std::uint8_t>());
}


void Interpreter::do_copy_16()
{
    operands().front()->write<std::uint16_t>(operands().back()->read<std::uint16_t>());
}


void Interpreter::do_copy_32()
{
    operands().front()->write<std::uint32_t>(operands().back()->read<std::uint32_t>());
}


void Interpreter::do_copy_64()
{
    operands().front()->write<std::uint64_t>(operands().back()->read<std::uint64_t>());
}


void Interpreter::do_copy()
{
    std::memcpy(operands().front()->start(), operands().back()->start(), operands().front()->count());
}


void Interpreter::do_memcpy()
{
    std::memcpy(operands().front()->read<MemPtr>(), operands().at(1)->read<MemPtr>(), operands().back()->as_size());
}


void Interpreter::do_memmove()
{
    std::memmove(operands().front()->read<MemPtr>(), operands().at(1)->read<MemPtr>(), operands().back()->as_size());
}


void Interpreter::do_memset()
{
    std::memset(operands().front()->read<MemPtr>(), (int)operands().at(1)->read<std::uint8_t>(), operands().back()->as_size());
}


void Interpreter::do_moveptr()
{
    auto const shift = operands().at(2)->as_shift() * (std::int64_t)operands().back()->as_shift();
    operands().front()->write_shifted(operands().at(1)->start(), shift);
}


void Interpreter::do_alloca()
{
    auto const num_bytes{ operands().at(1)->as_size() * operands().at(2)->as_size() };
    if (!state().can_allocate(num_bytes))
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("[OUT OF MEMORY] Cannot allocate memory on stack for a variable.")
            );
        return;
    }
    state().stack_top().push_back_local_variable(num_bytes);
    state().update_current_values();
    operands().front()->write<MemPtr>(state().stack_top().locals().back().start());
}


void Interpreter::do_stacksave()
{
    operands().front()->write<MemPtr>(state().stack_top().locals().back().start());
}


void Interpreter::do_stackrestore()
{
    MemPtr const saved_top{ operands().front()->read<MemPtr>() };
    while (state().stack_top().locals().back().start() != saved_top)
        state().stack_top().pop_back_local_variable();
}


void Interpreter::do_malloc()
{
    if (!state().can_allocate(operands().back()->as_size()))
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("[OUT OF MEMORY] Cannot allocate memory on heap.")
            );
        return;
    }
    try
    {
        MemBlock mb{ state().pointer_model(), operands().back()->as_size() };
        state().heap_segment().insert({ mb.start(), mb });
        operands().front()->write<MemPtr>(mb.start());
    }
    catch(const std::exception&)
    {
        operands().front()->write<std::nullptr_t>(nullptr);
    }
}


void Interpreter::do_free()
{
    state().heap_segment().erase( operands().front()->read<MemPtr>() );
}


void Interpreter::do_add_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() + operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_add_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() + operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_add_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() + operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_add_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() + operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_add_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() + operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_add_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() + operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_add_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() + operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_add_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() + operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_add_f32()
{
    operands().front()->write<float>(operands().at(1)->read<float>() + operands().at(2)->read<float>());
}


void Interpreter::do_add_f64()
{
    operands().front()->write<double>(operands().at(1)->read<double>() + operands().at(2)->read<double>());
}


void Interpreter::do_sub_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() - operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_sub_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() - operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_sub_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() - operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_sub_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() - operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_sub_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() - operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_sub_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() - operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_sub_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() - operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_sub_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() - operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_sub_f32()
{
    operands().front()->write<float>(operands().at(1)->read<float>() - operands().at(2)->read<float>());
}


void Interpreter::do_sub_f64()
{
    operands().front()->write<double>(operands().at(1)->read<double>() - operands().at(2)->read<double>());
}


void Interpreter::do_mul_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() * operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_mul_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() * operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_mul_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() * operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_mul_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() * operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_mul_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() * operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_mul_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() * operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_mul_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() * operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_mul_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() * operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_mul_f32()
{
    operands().front()->write<float>(operands().at(1)->read<float>() * operands().at(2)->read<float>());
}


void Interpreter::do_mul_f64()
{
    operands().front()->write<double>(operands().at(1)->read<double>() * operands().at(2)->read<double>());
}


void Interpreter::do_div_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() / operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_div_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() / operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_div_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() / operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_div_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() / operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_div_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() / operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_div_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() / operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_div_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() / operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_div_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() / operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_div_f32()
{
    operands().front()->write<float>(operands().at(1)->read<float>() / operands().at(2)->read<float>());
}


void Interpreter::do_div_f64()
{
    operands().front()->write<double>(operands().at(1)->read<double>() / operands().at(2)->read<double>());
}


void Interpreter::do_rem_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() % operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_rem_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() % operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_rem_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() % operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_rem_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() % operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_rem_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() % operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_rem_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() % operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_rem_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() % operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_rem_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() % operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_and_8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() & operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_and_16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() & operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_and_32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() & operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_and_64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() & operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_or_8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() | operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_or_16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() | operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_or_32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() | operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_or_64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() | operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_xor_8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() ^ operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_xor_16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() ^ operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_xor_32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() ^ operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_xor_64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() ^ operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_shl_8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() << operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_shl_16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() << operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_shl_32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() << operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_shl_64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() << operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_shr_s8()
{
    operands().front()->write<std::int8_t>(operands().at(1)->read<std::int8_t>() >> operands().at(2)->read<std::int8_t>());
}


void Interpreter::do_shr_s16()
{
    operands().front()->write<std::int16_t>(operands().at(1)->read<std::int16_t>() >> operands().at(2)->read<std::int16_t>());
}


void Interpreter::do_shr_s32()
{
    operands().front()->write<std::int32_t>(operands().at(1)->read<std::int32_t>() >> operands().at(2)->read<std::int32_t>());
}


void Interpreter::do_shr_s64()
{
    operands().front()->write<std::int64_t>(operands().at(1)->read<std::int64_t>() >> operands().at(2)->read<std::int64_t>());
}


void Interpreter::do_shr_u8()
{
    operands().front()->write<std::uint8_t>(operands().at(1)->read<std::uint8_t>() >> operands().at(2)->read<std::uint8_t>());
}


void Interpreter::do_shr_u16()
{
    operands().front()->write<std::uint16_t>(operands().at(1)->read<std::uint16_t>() >> operands().at(2)->read<std::uint16_t>());
}


void Interpreter::do_shr_u32()
{
    operands().front()->write<std::uint32_t>(operands().at(1)->read<std::uint32_t>() >> operands().at(2)->read<std::uint32_t>());
}


void Interpreter::do_shr_u64()
{
    operands().front()->write<std::uint64_t>(operands().at(1)->read<std::uint64_t>() >> operands().at(2)->read<std::uint64_t>());
}


void Interpreter::do_neg_f32()
{
    operands().front()->write<float>(-operands().back()->read<float>());
}


void Interpreter::do_neg_f64()
{
    operands().front()->write<double>(-operands().back()->read<double>());
}


void Interpreter::do_extend_s8_s16()
{
    operands().front()->write<std::int16_t>((std::int16_t)operands().back()->read<std::int8_t>());
}


void Interpreter::do_extend_s8_s32()
{
    operands().front()->write<std::int32_t>((std::int32_t)operands().back()->read<std::int8_t>());
}


void Interpreter::do_extend_s8_s64()
{
    operands().front()->write<std::int64_t>((std::int64_t)operands().back()->read<std::int8_t>());
}


void Interpreter::do_extend_s16_s32()
{
    operands().front()->write<std::int32_t>((std::int32_t)operands().back()->read<std::int16_t>());
}


void Interpreter::do_extend_s16_s64()
{
    operands().front()->write<std::int64_t>((std::int64_t)operands().back()->read<std::int16_t>());
}


void Interpreter::do_extend_s32_s64()
{
    operands().front()->write<std::int64_t>((std::int64_t)operands().back()->read<std::int32_t>());
}


void Interpreter::do_extend_u8_u16()
{
    operands().front()->write<std::uint16_t>((std::uint16_t)operands().back()->read<std::uint8_t>());
}


void Interpreter::do_extend_u8_u32()
{
    operands().front()->write<std::uint32_t>((std::uint32_t)operands().back()->read<std::uint8_t>());
}


void Interpreter::do_extend_u8_u64()
{
    operands().front()->write<std::uint64_t>((std::uint64_t)operands().back()->read<std::uint8_t>());
}


void Interpreter::do_extend_u16_u32()
{
    operands().front()->write<std::uint32_t>((std::uint32_t)operands().back()->read<std::uint16_t>());
}


void Interpreter::do_extend_u16_u64()
{
    operands().front()->write<std::uint64_t>((std::uint64_t)operands().back()->read<std::uint16_t>());
}


void Interpreter::do_extend_u32_u64()
{
    operands().front()->write<std::uint64_t>((std::uint64_t)operands().back()->read<std::uint32_t>());
}


void Interpreter::do_extend_f32_f64()
{
    operands().front()->write<double>((double)operands().back()->read<float>());
}


void Interpreter::do_truncate_u64_u32()
{
    operands().front()->write<std::uint32_t>((std::uint32_t)operands().back()->read<std::uint64_t>());
}


void Interpreter::do_truncate_u64_u16()
{
    operands().front()->write<std::uint16_t>((std::uint16_t)operands().back()->read<std::uint64_t>());
}


void Interpreter::do_truncate_u64_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)operands().back()->read<std::uint64_t>());
}


void Interpreter::do_truncate_u32_u16()
{
    operands().front()->write<std::uint16_t>((std::uint16_t)operands().back()->read<std::uint32_t>());
}


void Interpreter::do_truncate_u32_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)operands().back()->read<std::uint32_t>());
}


void Interpreter::do_truncate_u16_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)operands().back()->read<std::uint16_t>());
}


void Interpreter::do_truncate_f64_f32()
{
    operands().front()->write<float>((float)operands().back()->read<double>());
}


void Interpreter::do_f2i_f32_s8()
{
    operands().front()->write<std::int8_t>((std::int8_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_s16()
{
    operands().front()->write<std::int16_t>((std::int16_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_s32()
{
    operands().front()->write<std::int32_t>((std::int32_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_s64()
{
    operands().front()->write<std::int64_t>((std::int64_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_u16()
{
    operands().front()->write<std::uint16_t>((std::uint16_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_u32()
{
    operands().front()->write<std::uint32_t>((std::uint32_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f32_u64()
{
    operands().front()->write<std::uint64_t>((std::uint64_t)operands().back()->read<float>());
}


void Interpreter::do_f2i_f64_s8()
{
    operands().front()->write<std::int8_t>((std::int8_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_s16()
{
    operands().front()->write<std::int16_t>((std::int16_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_s32()
{
    operands().front()->write<std::int32_t>((std::int32_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_s64()
{
    operands().front()->write<std::int64_t>((std::int64_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_u16()
{
    operands().front()->write<std::uint16_t>((std::uint16_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_u32()
{
    operands().front()->write<std::uint32_t>((std::uint32_t)operands().back()->read<double>());
}


void Interpreter::do_f2i_f64_u64()
{
    operands().front()->write<std::uint64_t>((std::uint64_t)operands().back()->read<double>());
}


void Interpreter::do_i2f_s8_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::int8_t>());
}


void Interpreter::do_i2f_s8_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::int8_t>());
}


void Interpreter::do_i2f_s16_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::int16_t>());
}


void Interpreter::do_i2f_s16_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::int16_t>());
}


void Interpreter::do_i2f_s32_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::int32_t>());
}


void Interpreter::do_i2f_s32_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::int32_t>());
}


void Interpreter::do_i2f_s64_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::int64_t>());
}


void Interpreter::do_i2f_s64_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::int64_t>());
}


void Interpreter::do_i2f_u8_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::uint8_t>());
}


void Interpreter::do_i2f_u8_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::uint8_t>());
}


void Interpreter::do_i2f_u16_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::uint16_t>());
}


void Interpreter::do_i2f_u16_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::uint16_t>());
}


void Interpreter::do_i2f_u32_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::uint32_t>());
}


void Interpreter::do_i2f_u32_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::uint32_t>());
}


void Interpreter::do_i2f_u64_f32()
{
    operands().front()->write<float>((float)operands().back()->read<std::uint64_t>());
}


void Interpreter::do_i2f_u64_f64()
{
    operands().front()->write<double>((double)operands().back()->read<std::uint64_t>());
}


void Interpreter::do_p2i_8()
{
    operands().front()->write_pointer_as_uint8(operands().back()->read<MemPtr>());
}


void Interpreter::do_p2i_16()
{
    operands().front()->write_pointer_as_uint16(operands().back()->read<MemPtr>());
}


void Interpreter::do_p2i_32()
{
    operands().front()->write_pointer_as_uint32(operands().back()->read<MemPtr>());
}


void Interpreter::do_p2i_64()
{
    operands().front()->write_pointer_as_uint64(operands().back()->read<MemPtr>());
}


void Interpreter::do_i2p_8()
{
    operands().front()->write_uint8_as_pointer(operands().back()->read<std::uint8_t>());
}


void Interpreter::do_i2p_16()
{
    operands().front()->write_uint16_as_pointer(operands().back()->read<std::uint16_t>());
}


void Interpreter::do_i2p_32()
{
    operands().front()->write_uint32_as_pointer(operands().back()->read<std::uint32_t>());
}


void Interpreter::do_i2p_64()
{
    operands().front()->write_uint64_as_pointer(operands().back()->read<std::uint64_t>());
}


void Interpreter::do_less_s8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int8_t>() < operands().at(2)->read<std::int8_t>()));
}


void Interpreter::do_less_s16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int16_t>() < operands().at(2)->read<std::int16_t>()));
}


void Interpreter::do_less_s32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int32_t>() < operands().at(2)->read<std::int32_t>()));
}


void Interpreter::do_less_s64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int64_t>() < operands().at(2)->read<std::int64_t>()));
}


void Interpreter::do_less_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() < operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_less_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() < operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_less_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() < operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_less_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() < operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_less_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() < operands().at(2)->read<float>()));
}


void Interpreter::do_less_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() < operands().at(2)->read<double>()));
}


void Interpreter::do_less_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f < g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_less_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f < g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_less_equal_s8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int8_t>() <= operands().at(2)->read<std::int8_t>()));
}


void Interpreter::do_less_equal_s16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int16_t>() <= operands().at(2)->read<std::int16_t>()));
}


void Interpreter::do_less_equal_s32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int32_t>() <= operands().at(2)->read<std::int32_t>()));
}


void Interpreter::do_less_equal_s64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int64_t>() <= operands().at(2)->read<std::int64_t>()));
}


void Interpreter::do_less_equal_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() <= operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_less_equal_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() <= operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_less_equal_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() <= operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_less_equal_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() <= operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_less_equal_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() <= operands().at(2)->read<float>()));
}


void Interpreter::do_less_equal_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() <= operands().at(2)->read<double>()));
}


void Interpreter::do_less_equal_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f <= g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_less_equal_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f <= g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_greater_s8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int8_t>() > operands().at(2)->read<std::int8_t>()));
}


void Interpreter::do_greater_s16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int16_t>() > operands().at(2)->read<std::int16_t>()));
}


void Interpreter::do_greater_s32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int32_t>() > operands().at(2)->read<std::int32_t>()));
}


void Interpreter::do_greater_s64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int64_t>() > operands().at(2)->read<std::int64_t>()));
}


void Interpreter::do_greater_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() > operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_greater_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() > operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_greater_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() > operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_greater_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() > operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_greater_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() > operands().at(2)->read<float>()));
}


void Interpreter::do_greater_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() > operands().at(2)->read<double>()));
}


void Interpreter::do_greater_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f > g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_greater_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f > g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_greater_equal_s8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int8_t>() >= operands().at(2)->read<std::int8_t>()));
}


void Interpreter::do_greater_equal_s16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int16_t>() >= operands().at(2)->read<std::int16_t>()));
}


void Interpreter::do_greater_equal_s32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int32_t>() >= operands().at(2)->read<std::int32_t>()));
}


void Interpreter::do_greater_equal_s64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::int64_t>() >= operands().at(2)->read<std::int64_t>()));
}


void Interpreter::do_greater_equal_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() >= operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_greater_equal_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() >= operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_greater_equal_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() >= operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_greater_equal_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() >= operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_greater_equal_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() >= operands().at(2)->read<float>()));
}


void Interpreter::do_greater_equal_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() >= operands().at(2)->read<double>()));
}


void Interpreter::do_greater_equal_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f >= g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_greater_equal_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f >= g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_equal_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() == operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_equal_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() == operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_equal_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() == operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_equal_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() == operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_equal_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() == operands().at(2)->read<float>()));
}


void Interpreter::do_equal_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() == operands().at(2)->read<double>()));
}


void Interpreter::do_equal_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f == g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_equal_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f == g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_unequal_u8()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint8_t>() != operands().at(2)->read<std::uint8_t>()));
}


void Interpreter::do_unequal_u16()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint16_t>() != operands().at(2)->read<std::uint16_t>()));
}


void Interpreter::do_unequal_u32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint32_t>() != operands().at(2)->read<std::uint32_t>()));
}


void Interpreter::do_unequal_u64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<std::uint64_t>() != operands().at(2)->read<std::uint64_t>()));
}


void Interpreter::do_unequal_f32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<float>() != operands().at(2)->read<float>()));
}


void Interpreter::do_unequal_f64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)(operands().at(1)->read<double>() != operands().at(2)->read<double>()));
}


void Interpreter::do_unequal_w32()
{
    auto const f{ operands().at(1)->read<float>() };
    auto const g{ operands().at(2)->read<float>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f != g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_unequal_w64()
{
    auto const f{ operands().at(1)->read<double>() };
    auto const g{ operands().at(2)->read<double>() };
    bool const cond{ std::isnan(f) || std::isnan(g) || f != g };
    operands().front()->write<std::uint8_t>((std::uint8_t)cond);
}


void Interpreter::do_isnan_w32()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)std::isnan(operands().back()->read<float>()));
}


void Interpreter::do_isnan_w64()
{
    operands().front()->write<std::uint8_t>((std::uint8_t)std::isnan(operands().back()->read<double>()));
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
        MemBlock mb{ state().pointer_model(), array_size };
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

    platform_linux_64_bit::va_list* const va_list_ptr{ (platform_linux_64_bit::va_list*)operands().front()->read<MemPtr>() };
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

    platform_linux_64_bit::va_list* const va_list_ptr{ (platform_linux_64_bit::va_list*)operands().front()->read<MemPtr>() };
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


void Interpreter::do_va_copy()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // IMPORTANT: This implementation is valid only for programs targeted to Linux 64-bit platform.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    NOT_IMPLEMENTED_YET();
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
        func_idx = state().functions_at_addresses().at(operands().front()->read<MemPtr>());

    Function const& func = program().functions().at(func_idx);

    if (!state().can_allocate(func.initial_stack_bytes()))
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("[OUT OF MEMORY] Cannot allocate memory on stack for called function.")
            );
        return;
    }
    if (!state().has_free_segments(func.parameters().size() + func.local_variables().size()))
    {
        state().set_stage(ExecState::Stage::FINISHED);
        state().set_termination(
            ExecState::Termination::ERROR,
            "sala::Interpreter",
            state().make_error_message("[OUT OF MEMORY] Not enough free segments for stack variables of called function.")
            );
        return;
    }

    state().stack_top().ip().next();

    state().stack_segment().push_back(StackRecord(state().pointer_model(), func));

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

    extern_code_->call_code_of_current_function_if_registered_external();
}


void Interpreter::do_ret()
{
    state().stack_segment().pop_back();
}


}
