#include <sala/exec_state.hpp>
#include <sala/pointer_model_default.hpp>
#include <sala/pointer_model_m32.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <sstream>

namespace sala::detail {


template<typename T>
std::size_t count_segment_bytes(std::vector<T> const& items)
{
    std::size_t sum{ 0ULL };
    for (auto const& item : items)
        sum += item.num_bytes();
    return sum;
}


template<typename T>
MemBlock allocate_segment(PointerModel* const pointer_model, std::vector<T> const& items)
{
    std::size_t const num_bytes{ count_segment_bytes(items) };
    return MemBlock{ pointer_model, new std::uint8_t[num_bytes], num_bytes, 0U };
}


template<typename T>
void map_items_to_allocated_segment(
    PointerModel* const pointer_model,
    MemBlock const& block,
    std::vector<T> const& items,
    std::vector<MemBlock>& blocks_for_items
    )
{
    ASSUMPTION(blocks_for_items.empty());
    MemPtr ptr{ block.start() };
    for (auto const& item : items)
    {
        ASSUMPTION(ptr + item.num_bytes() <= block.start() + block.count());
        blocks_for_items.push_back(MemBlock{ pointer_model, ptr, item.num_bytes() });
        ptr += item.num_bytes();
    }
    INVARIANT(ptr == block.start() + block.count());
}


}

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
    , argv_{ pointer_model_, std::max(1, argc + 1) * pointer_model_->sizeof_pointer() }
    , argv_c_strings_{}
    , warnings_{}

    , constant_segment_memory_block_{ detail::allocate_segment(pointer_model_, program().constants()) }
    , static_segment_memory_block_{ detail::allocate_segment(pointer_model_, program().static_variables()) }
    , function_segment_memory_block_{ detail::allocate_segment(pointer_model_, program().functions()) }

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
        memory_size_in_bytes == 0ULL ||
        [this]() -> std::size_t {
            std::size_t arg_bytes{ sizeof(int) + argc_ * pointer_model_->sizeof_pointer() };
            for (MemBlock const& block : argv_c_strings_)
                arg_bytes += block.count();
            return arg_bytes;
        }() <= memory_size_in_bytes
    ));

    detail::map_items_to_allocated_segment(pointer_model(), constant_segment_memory_block(), program().constants(), constant_segment_);
    detail::map_items_to_allocated_segment(pointer_model(), static_segment_memory_block(), program().static_variables(), static_segment_);
    detail::map_items_to_allocated_segment(pointer_model(), function_segment_memory_block(), program().functions(), function_segment_);

    for (std::size_t i = 0ULL; i < program().constants().size(); ++i)
    {
        auto const& constant{ program().constants().at(i) };
        INVARIANT(constant_segment().at(i).count() == constant.num_bytes());
        std::memcpy(constant_segment().at(i).start(), constant.bytes().data(), constant.num_bytes());
    }

    for (std::size_t i = 0ULL; i < program().functions().size(); ++i)
    {
        auto const& func{ program().functions().at(i) };
        INVARIANT(function_segment().at(i).count() == func.num_bytes());
        functions_at_addresses_.insert({ function_segment().at(i).start(), func.index() });
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
