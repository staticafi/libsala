#include <sala/exec_state.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <sstream>

namespace sala {


MemBlock::MemBlock()
    : start_{ nullptr }
    , count_{ 0ULL }
{}


MemBlock::MemBlock(std::size_t const num_bytes, std::uint8_t init_value)
    : start_{ new std::uint8_t[num_bytes] }
    , count_{ num_bytes }
{
    std::memset(start(), init_value, count());
}


std::size_t MemBlock::as_size() const
{
    switch (count())
    {
    case 1ULL: return (std::size_t)*start();
    case 2ULL: return (std::size_t)*(std::uint16_t*)start();
    case 4ULL: return (std::size_t)*(std::uint32_t*)start();
    case 8ULL: return (std::size_t)*(std::uint64_t*)start();
    default: UNREACHABLE(); return 0ULL;
    }
}


InstrPointer::InstrPointer()
    : block_{ 0U }
    , instr_{ 0U }
{}


void InstrPointer::next()
{
    ++instr_;
}


void InstrPointer::jump(std::uint32_t const new_block_idx)
{
    block_ = new_block_idx;
    instr_ = 0U;
}


StackRecord::StackRecord()
    : function_index_{ 0U }
    , ip_{}
    , parameters_{}
    , locals_{}
    , variadic_parameters_{}
{}


StackRecord::StackRecord(Function const& F)
    : function_index_{ F.index() }
    , ip_{}
    , parameters_{}
    , locals_{}
    , variadic_parameters_{}
{
    for (auto const& param : F.parameters())
        parameters_.push_back(MemBlock{ param.num_bytes() });
    for (auto const& local : F.local_variables())
        locals_.push_back(MemBlock{ local.num_bytes() });
}


void StackRecord::push_back_variadic_parameter(std::size_t const num_bytes)
{
    variadic_parameters_.push_back(MemBlock{ num_bytes });
}


void StackRecord::push_back_local_variable(std::size_t num_bytes)
{
    locals_.push_back(MemBlock{ num_bytes });
}


void StackRecord::pop_back_local_variable()
{
    locals_.pop_back();
}


ExecState::ExecState(Program const* const P)
    : program_{ P }

    , stage_{ Stage::INITIALIZING }
    , termination_{ Termination::UNKNOWN }
    , terminator_{}
    , error_message_{}
    , exit_code_{ sizeof(std::uint64_t) }

    , constant_segment_{}
    , static_segment_{}
    , function_segment_{}
    , functions_at_addresses_{}
    , stack_segment_{}
    , heap_segment_{}

    , stack_exit_depth_{ 0ULL }

    , atexit_stack_{}

    , current_function_{ nullptr }
    , current_block_{ nullptr }
    , current_instruction_{ nullptr }
    , current_operands_{}
{
    for (auto const& constant : program().constants())
    {
        constant_segment_.push_back(MemBlock{ constant.num_bytes() });
        std::memcpy(constant_segment_.back().start(), constant.bytes().data(), constant.num_bytes());
    }

    for (auto const& var : program().static_variables())
        static_segment_.push_back(MemBlock{ var.num_bytes(), 0 });

    for (auto const& func : program().functions())
    {
        ASSUMPTION((std::uint32_t)function_segment_.size() == func.index());
        function_segment_.push_back(MemBlock{ 1ULL });
        functions_at_addresses_.insert({ function_segment_.back().start(), func.index() });
    }

    stack_segment_.push_back(StackRecord(program().functions().at(Program::static_initializer())));

    update_current_values();
}


bool ExecState::set_stage(Stage const type)
{
    if (type <= stage_)
        return false;
    stage_ = type;
    return true;
}


bool ExecState::set_termination(Termination const type, std::string const& terminator, std::string const& message)
{
    if (type <= termination_)
        return false;
    termination_ = type;
    terminator_ = terminator;
    error_message_ = message;
    return true;
}


void ExecState::update_current_values()
{
    current_function_ = &program().functions().at(stack_top().function_index());
    current_block_ = &current_function_->basic_blocks().at(stack_top().ip().block());
    current_instruction_ = &current_block_->instructions().at(stack_top().ip().instr());

    current_operands_.clear();
    for (std::uint32_t i = 0U, n = (std::uint32_t)current_instruction_->operands().size(); i < n; ++i)
    {
        auto const idx{ current_instruction_->operands().at(i) };
        switch (current_instruction_->descriptors().at(i))
        {
        case Instruction::Descriptor::STATIC:
            current_operands_.push_back(&static_segment().at(idx));
            break;
        case Instruction::Descriptor::LOCAL:
            current_operands_.push_back(&stack_top().locals().at(idx));
            break;
        case Instruction::Descriptor::PARAMETER:
            current_operands_.push_back(&stack_top().parameters().at(idx));
            break;
        case Instruction::Descriptor::CONSTANT:
            current_operands_.push_back(&constant_segment().at(idx));
            break;
        case Instruction::Descriptor::FUNCTION:
            current_operands_.push_back(&function_segment().at(idx));
            break;
        default: UNREACHABLE(); break;
        }
    }
}


std::string ExecState::current_location_message() const
{
    auto& backmapping{
        program().functions().at(stack_top().function_index())
                 .basic_blocks().at(ip().block())
                 .instructions().at(ip().instr()).source_back_mapping()
        };

    std::stringstream sstr;
    sstr << "function " << stack_top().function_index()
         << ", block " << ip().block()
         << ", instruction " << ip().instr()
         << ", backmapping [" << backmapping.line << ',' << backmapping.column << "]"
         ;

    return sstr.str();
}


std::string ExecState::make_error_message(std::string const& text) const
{
    std::stringstream sstr;
    sstr << "In " << current_location_message() << ": " << text;
    return sstr.str();
}


}
