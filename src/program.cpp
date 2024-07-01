#include <sala/program.hpp>
#include <utility/assumptions.hpp>
#include <type_traits>

namespace sala {


void Instruction::push_back_operand(std::uint32_t const operand, Descriptor const descriptor)
{
    operands_.push_back(operand);
    descriptors_.push_back(descriptor);
}


void Instruction::assign(Instruction const& other)
{
    opcode_ = other.opcode();
    modifier_ = other.modifier();
    operands_ = other.operands();
    descriptors_ = other.descriptors();
    back_mapping_ = other.source_back_mapping();
}


Instruction& BasicBlock::push_back_instruction()
{
    instructions_.push_back({});
    instructions_.back().set_basic_block_index(index());
    instructions_.back().set_index((std::uint32_t)instructions_.size() - 1U);
    return instructions_.back();

}


void BasicBlock::pop_back_instruction()
{
    instructions_.pop_back();
}


void BasicBlock::assign_instruction(std::size_t const index, Instruction const& instruction)
{
    instructions_.at(index).assign(instruction);
}


std::size_t Function::initial_stack_bytes() const
{
    if (initial_stack_bytes_ == std::numeric_limits<std::size_t>::max())
    {
        initial_stack_bytes_ = 0ULL;
        for (auto const& param : parameters())
            initial_stack_bytes_ += param.num_bytes();
        for (auto const& local : local_variables())
            initial_stack_bytes_ += local.num_bytes();
    }
    return initial_stack_bytes_;
}


BasicBlock& Function::push_back_basic_block()
{
    blocks_.push_back({});
    blocks_.back().set_function_index(index());
    blocks_.back().set_index((std::uint32_t)blocks_.size() - 1U);
    return blocks_.back();
}


Variable& Function::push_back_parameter()
{
    parameters_.push_back({});
    parameters_.back().set_program(program());
    parameters_.back().set_function_index(index());
    parameters_.back().set_index((std::uint32_t)parameters_.size() - 1U);
    parameters_.back().set_region(Variable::Region::STACK);
    initial_stack_bytes_ = std::numeric_limits<std::size_t>::max();
    return parameters_.back();    
}


Variable& Function::push_back_local_variable()
{
    locals_.push_back({});
    locals_.back().set_program(program());
    locals_.back().set_function_index(index());
    locals_.back().set_index((std::uint32_t)locals_.size() - 1U);
    locals_.back().set_region(Variable::Region::STACK);
    initial_stack_bytes_ = std::numeric_limits<std::size_t>::max();
    return locals_.back();    
}


Program::Program()
    : version_{ "0.1" }
    , num_cpu_bits_{ 64U }
    , name_{}
    , entry_function_{ 0U }
    , functions_{}
    , variables_{}
    , constants_{}
    , external_variables_{}
    , external_functions_{}
{}


std::uint32_t Program::static_initializer() { return 0U; }
std::string Program::static_initializer_name() { return "__sala_static_initializer__"; }


Function& Program::push_back_function(std::string const& func_name)
{
    functions_.push_back({});
    functions_.back().set_program(this);
    functions_.back().set_name(func_name);
    functions_.back().set_index((std::uint32_t)functions_.size() - 1U);
    return functions_.back();
}


Variable& Program::push_back_static_variable()
{
    variables_.push_back({});
    variables_.back().set_program(this);
    variables_.back().set_function_index(std::numeric_limits<std::uint32_t>::max());
    variables_.back().set_index((std::uint32_t)variables_.size() - 1U);
    variables_.back().set_region(Variable::Region::STATIC);
    return variables_.back();    
}


Constant& Program::push_back_constant()
{
    constants_.push_back({});
    constants_.back().set_program(this);
    constants_.back().set_index((std::uint32_t)constants_.size() - 1U);
    return constants_.back();
}


void Program::push_back_external_variable(std::uint32_t const index, std::string const& name)
{
    external_variables_.push_back({ index, name });
    variables_.at(index).set_external(true);
}


void Program::push_back_external_function(std::uint32_t const index)
{
    external_functions_.push_back(index);
    functions_.at(index).set_external(true);
}


}
