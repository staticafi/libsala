#include <sala/streaming.hpp>
#include <sala/program.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <boost/json.hpp>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iterator>

namespace sala {


static Instruction::Opcode instruction_opcode_from_name(std::string const& opcode_name)
{
    static std::unordered_map<std::string, Instruction::Opcode> const map {
        { "__INVALID__", Instruction::Opcode::__INVALID__ },
        { "NOP", Instruction::Opcode::NOP },
        { "HALT", Instruction::Opcode::HALT },
        { "ADDRESS", Instruction::Opcode::ADDRESS },
        { "LOAD", Instruction::Opcode::LOAD },
        { "STORE", Instruction::Opcode::STORE },
        { "COPY", Instruction::Opcode::COPY },
        { "MEMCPY", Instruction::Opcode::MEMCPY },
        { "MEMMOVE", Instruction::Opcode::MEMMOVE },
        { "MEMSET", Instruction::Opcode::MEMSET },
        { "MOVEPTR", Instruction::Opcode::MOVEPTR },
        { "ALLOCA", Instruction::Opcode::ALLOCA },
        { "STACKSAVE", Instruction::Opcode::STACKSAVE },
        { "STACKRESTORE", Instruction::Opcode::STACKRESTORE },
        { "MALLOC", Instruction::Opcode::MALLOC },
        { "FREE", Instruction::Opcode::FREE },
        { "ADD", Instruction::Opcode::ADD },
        { "SUB", Instruction::Opcode::SUB },
        { "MUL", Instruction::Opcode::MUL },
        { "DIV", Instruction::Opcode::DIV },
        { "REM", Instruction::Opcode::REM },
        { "AND", Instruction::Opcode::AND },
        { "OR", Instruction::Opcode::OR },
        { "XOR", Instruction::Opcode::XOR },
        { "SHL", Instruction::Opcode::SHL },
        { "SHR", Instruction::Opcode::SHR },
        { "NEG", Instruction::Opcode::NEG },
        { "EXTEND", Instruction::Opcode::EXTEND },
        { "TRUNCATE", Instruction::Opcode::TRUNCATE },
        { "F2I", Instruction::Opcode::F2I },
        { "I2F", Instruction::Opcode::I2F },
        { "P2I", Instruction::Opcode::P2I },
        { "I2P", Instruction::Opcode::I2P },
        { "LESS", Instruction::Opcode::LESS },
        { "LESS_EQUAL", Instruction::Opcode::LESS_EQUAL },
        { "GREATER", Instruction::Opcode::GREATER },
        { "GREATER_EQUAL", Instruction::Opcode::GREATER_EQUAL },
        { "EQUAL", Instruction::Opcode::EQUAL },
        { "UNEQUAL", Instruction::Opcode::UNEQUAL },
        { "JUMP", Instruction::Opcode::JUMP },
        { "BRANCH", Instruction::Opcode::BRANCH },
        { "CALL", Instruction::Opcode::CALL },
        { "RET", Instruction::Opcode::RET },
        { "VA_START", Instruction::Opcode::VA_START },
        { "VA_END", Instruction::Opcode::VA_END },
        { "VA_ARG", Instruction::Opcode::VA_ARG },
    };
    return map.at(opcode_name);
}


static Instruction::Modifier instruction_modifier_from_name(char const modifier_name)
{
    static std::unordered_map<char, Instruction::Modifier> const map {
        { 'n', Instruction::Modifier::NONE },
        { 's', Instruction::Modifier::SIGNED },
        { 'u', Instruction::Modifier::UNSIGNED },
        { 'f', Instruction::Modifier::FLOATING },
    };
    return map.at(modifier_name);
}


static Instruction::Descriptor instruction_descriptor_from_name(char const descriptor_name)
{
    static std::unordered_map<char, Instruction::Descriptor> const map {
        { 's', Instruction::Descriptor::STATIC },
        { 'l', Instruction::Descriptor::LOCAL },
        { 'p', Instruction::Descriptor::PARAMETER },
        { 'c', Instruction::Descriptor::CONSTANT },
        { 'f', Instruction::Descriptor::FUNCTION },
    };
    return map.at(descriptor_name);
}


static SourceBackMapping parse_back_mapping(boost::json::value const&  json_value)
{
    auto const& json_array = json_value.as_array();
    return { (std::uint32_t)json_array.front().as_int64(), (std::uint32_t)json_array.back().as_int64() };
}


static void parse_variable(Variable& sala_variable, boost::json::value const&  json_value)
{
    auto const& json_array = json_value.as_array();
    sala_variable.set_num_bytes((std::size_t)json_array.front().as_int64());
    sala_variable.source_back_mapping() = parse_back_mapping(json_array.back());
}


static void parse_instruction(Instruction& sala_instruction, boost::json::value const&  json_value)
{
    auto const& json_array = json_value.as_array();
    sala_instruction.set_opcode(instruction_opcode_from_name(json_array.front().as_string().c_str()));
    sala_instruction.set_modifier(instruction_modifier_from_name(json_array.at(1).as_string()[0]));
    sala_instruction.source_back_mapping() = parse_back_mapping(json_array.back());
    if (json_array.size() == 3ULL)
        return; // Instruction with no operands => we are done parsing the instruction.
    auto const& descriptor_names = json_array.at(2).as_string();
    for (std::size_t i = 0, n = descriptor_names.size(); i < n; ++i)
        sala_instruction.push_back_operand(
            (std::uint32_t)json_array.at(3ULL + i).as_int64(),
            instruction_descriptor_from_name(descriptor_names.at(i))
            );
}


std::istream& operator>>(std::istream& istr, Program& program)
{
    boost::json::error_code error_code;
    boost::json::value const json_program{ boost::json::parse(std::string(std::istreambuf_iterator<char>(istr), {}), error_code) };

    boost::json::object const& root_obj = json_program.as_object();
    ASSUMPTION(root_obj.at("magic").as_string() == "sala");
    ASSUMPTION(root_obj.at("version").as_string() == "0.1");
    std::uint16_t const num_cpu_bits = (std::uint16_t)root_obj.at("num_cpu_bits").as_int64();
    ASSUMPTION(num_cpu_bits == 32 || num_cpu_bits == 64);
    program.set_system(root_obj.at("system").as_string().c_str());
    program.set_num_cpu_bits(num_cpu_bits);
    program.set_name(root_obj.at("name").as_string().c_str());
    program.set_entry_function((std::uint32_t)root_obj.at("entry_function").as_int64());

    for (auto const& constant : json_program.at("constants").as_array())
    {
        auto& sala_constant = program.push_back_constant();

        std::string const str_value{ constant.as_string().c_str() };
        ASSUMPTION(str_value.size() % 2 == 0);
        for (std::size_t i = 0ULL; i < str_value.size(); i += 2)
        {
            std::stringstream sstr;
            sstr << std::hex << str_value.at(i) << std::hex << str_value.at(i + 1ULL);
            std::uint32_t value;
            sstr >> value;
            INVARIANT(value < 256U);
            sala_constant.push_back_byte((std::uint8_t)value);
        }
    }

    for (auto const& static_var_value : json_program.at("static").as_array())
        parse_variable(program.push_back_static_variable(), static_var_value.as_array());

    for (auto const& function_value : json_program.at("functions").as_array())
    {
        auto const& function_obj = function_value.as_object();

        auto& sala_function = program.push_back_function(function_obj.at("name").as_array().front().as_string().c_str());

        sala_function.source_back_mapping() = parse_back_mapping(function_obj.at("name").as_array().back());

        for (auto const& var_value : function_obj.at("parameters").as_array())
            parse_variable(sala_function.push_back_parameter(), var_value.as_array());
        for (auto const& var_value : function_obj.at("locals").as_array())
            parse_variable(sala_function.push_back_local_variable(), var_value.as_array());

        for (auto const& block_value : function_obj.at("basic_blocks").as_array())
        {
            auto& sala_block = sala_function.push_back_basic_block();

            auto const& block_obj = block_value.as_object();
            for (auto const& array_value : block_obj.at("instructions").as_array())
                parse_instruction(sala_block.push_back_instruction(), array_value);

            for (auto const& array_value : block_obj.at("successors").as_array())
                sala_block.push_back_successor((std::uint32_t)array_value.as_int64());
        }
    }

    for (auto const& external_var_value : json_program.at("external_variables").as_array())
        program.push_back_external_variable(
            (std::uint32_t)external_var_value.as_array().front().as_int64(),
            external_var_value.as_array().back().as_string().c_str()
            );

    for (auto const& external_func_value : json_program.at("external_functions").as_array())
        program.push_back_external_function((std::uint32_t)external_func_value.as_int64());

    return istr;
}


}