#include <sala/exec_state.hpp>
#include <sala/pointer_model_default.hpp>
#include <sala/pointer_model_m32.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <sstream>

namespace sala {


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


StackRecord::StackRecord(PointerModel* const pointer_model, Function const& F)
    : pointer_model_{ pointer_model }
    , function_index_{ F.index() }
    , ip_{}
    , parameters_{}
    , locals_{}
    , variadic_parameters_{}
{
    for (auto const& param : F.parameters())
        parameters_.push_back(MemBlock{ pointer_model_, param.num_bytes() });
    for (auto const& local : F.local_variables())
        locals_.push_back(MemBlock{ pointer_model_, local.num_bytes() });
}


void StackRecord::push_back_variadic_parameter(std::size_t const num_bytes)
{
    variadic_parameters_.push_back(MemBlock{ pointer_model_, num_bytes });
}


void StackRecord::push_back_local_variable(std::size_t num_bytes)
{
    locals_.push_back(MemBlock{ pointer_model_, num_bytes });
}


void StackRecord::pop_back_local_variable()
{
    locals_.pop_back();
}


ExecState::ExecState(Program const* const P, int const argc, char* argv[], std::size_t const memory_size_in_bytes)
    : program_{ P }
    , pointer_model_{
        program_->num_cpu_bits() == 32U ?
            (PointerModel*)new PointerModelM32_SegmentOffset() :
            (PointerModel*)new PointerModelDefault()
        }
    , memory_size_in_bytes_{ memory_size_in_bytes }

    , stage_{ Stage::INITIALIZING }
    , termination_{ Termination::UNKNOWN }
    , terminator_{}
    , error_message_{}
    , termination_instruction_{ nullptr }
    , exit_code_{ pointer_model_, sizeof(std::uint64_t) }
    , argc_{ argc }
    , argv_{ pointer_model_, std::max(1, argc) * pointer_model_->sizeof_pointer() }
    , argv_c_strings_{}
    , warnings_{}

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
    static_assert(sizeof(int) == sizeof(std::int32_t));
    ASSUMPTION(argc_ >= 0 && (argc_ == 0 || argv != nullptr));

    std::memset(argv_.start(), 0, argv_.count());
    argv_c_strings_.reserve(argc_);
    for (int i = 0; i < argc_; ++i)
    {
        std::size_t const len{ std::strlen(argv[i]) + 1ULL };
        argv_c_strings_.push_back(MemBlock{ pointer_model(), len });
        std::memcpy(argv_c_strings_.back().start(), argv[i], len);
        argv_.write_pointer_from_offset(i * pointer_model_->sizeof_pointer(), argv_c_strings_.back().start());
    }

    ASSUMPTION((
        [this]() -> std::size_t {
            std::size_t arg_bytes{ sizeof(int) + argc_ * pointer_model_->sizeof_pointer() };
            for (MemBlock const& block : argv_c_strings_)
                arg_bytes += block.count();
            return arg_bytes;
        }() <= memory_size_in_bytes
    ));

    for (auto const& constant : program().constants())
    {
        constant_segment_.push_back(MemBlock{ pointer_model(), constant.num_bytes() });
        std::memcpy(constant_segment_.back().start(), constant.bytes().data(), constant.num_bytes());
    }

    for (auto const& var : program().static_variables())
        static_segment_.push_back(MemBlock{ pointer_model(), var.num_bytes(), 0 });

    for (auto const& func : program().functions())
    {
        ASSUMPTION((std::uint32_t)function_segment_.size() == func.index());
        function_segment_.push_back(MemBlock{ pointer_model(), 1ULL });
        functions_at_addresses_.insert({ function_segment_.back().start(), func.index() });
    }

    stack_segment_.push_back(StackRecord(pointer_model(), program().functions().at(Program::static_initializer())));

    update_current_values();
}


ExecState::~ExecState()
{
    exit_code_ = {};
    argv_ = {};
    argv_c_strings_.clear();

    constant_segment_.clear();
    static_segment_.clear();
    function_segment_.clear();
    functions_at_addresses_.clear();
    stack_segment_.clear();
    heap_segment_.clear();

    current_operands_.clear();

    delete pointer_model_;
}


std::string  ExecState::report(std::string const&  error_message_suffix) const
{
    std::stringstream  sstr;
    sstr << "{ "
         << "\"stage\": \"" << to_string(stage()) << "\""
         << ", "
         << "\"exit_code\": " << exit_code()
         << ", "
         << "\"termination\": \"" << to_string(termination()) << "\""
         << ", "
         << "\"terminator\": \"" << terminator() << "\""
         << ", "
         << "\"error_message\": \"" << error_message() << error_message_suffix << "\""
         << " }"
         ;
    return sstr.str();
}


bool ExecState::set_stage(Stage const type)
{
    if (type <= stage_)
        return false;
    stage_ = type;
    return true;
}


bool ExecState::set_termination(
    Termination const type,
    std::string const& terminator,
    std::string const& message,
    Instruction const* const instruction
    )
{
    if (type <= termination_)
        return false;
    termination_ = type;
    terminator_ = terminator;
    error_message_ = message;
    termination_instruction_ = instruction != nullptr ? instruction : current_instruction_;
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


std::string  to_string(ExecState::Stage  stage)
{
    switch (stage)
    {
        case ExecState::Stage::INITIALIZING: return "INITIALIZING";
        case ExecState::Stage::EXECUTING: return "EXECUTING";
        case ExecState::Stage::TERMINATING: return "TERMINATING";
        case ExecState::Stage::FINISHED: return "FINISHED";
        default: return "UNDEFINED";
    }
}


std::string  to_string(ExecState::Termination  termination)
{
    switch (termination)
    {
        case ExecState::Termination::UNKNOWN: return "UNKNOWN";
        case ExecState::Termination::NORMAL: return "NORMAL";
        case ExecState::Termination::ERROR: return "ERROR";
        case ExecState::Termination::CRASH: return "CRASH";
        default: return "UNDEFINED";
    }
}


}
