#include <sala/streaming.hpp>
#include <sala/program.hpp>
#include <sala/streaming_utils.hpp>
#include <utility/invariants.hpp>
#include <iostream>
#include <iomanip>

namespace sala::detail {


static bool save_json_comments{ false };

void enable_json_comments(bool const state)
{
    save_json_comments = state;
}


}

namespace sala {


struct DbgLine { std::size_t line_; };
std::ostream& operator<<(std::ostream& ostr, DbgLine const& line)
{
    if (detail::save_json_comments)
        ostr << " // " << line.line_;
    return ostr;
}


static bool save_constants(std::ostream& ostr, std::vector<Constant> const& constants)
{
    for (std::size_t i = 0U; i < constants.size(); ++i)
    {
        if (i != 0U)
        {
            ostr << ',' << DbgLine{i - 1ULL};
            if (detail::save_json_comments)
                ostr << ", #" << constants.at(i - 1ULL).bytes().size();
            ostr << '\n';
        }
        ostr << "  \"" << bytes_to_hex_string(constants.at(i).bytes()) << "\"";
    }
    if (!constants.empty())
    {
        ostr << DbgLine{constants.size() - 1ULL};
        if (detail::save_json_comments)
            ostr << ", #" << constants.back().bytes().size();
    }
    return !constants.empty();
}


static bool save_variables(std::ostream& ostr, std::vector<Variable> const& variables, std::string const& shift)
{
    for (std::size_t i = 0U; i < variables.size(); ++i)
    {
        if (i != 0U)
            ostr << ',' << DbgLine{i - 1ULL} << '\n';
        auto const& variable = variables.at(i);
        ostr << shift << "[ " << variable.num_bytes() << ", " << source_back_mapping_to_string(variable.source_back_mapping()) << " ]";
    }
    if (!variables.empty())
        ostr << DbgLine{variables.size() - 1ULL};
    return !variables.empty();
}


std::ostream& operator<<(std::ostream& ostr, Instruction const& instruction)
{
    ostr << "[ "
            << "\"" << instruction_opcode_to_string(instruction.opcode()) << "\", "
            << "\"" << instruction_modifier_to_string(instruction.modifier()) << "\"";
    if (!instruction.descriptors().empty())
    {
        ostr << ", \"";
        for (auto descriptor : instruction.descriptors())
            ostr << instruction_descriptor_to_string(descriptor);
        ostr << "\"";
    }
    for (auto operand : instruction.operands())
        ostr << ", " << operand;
    ostr << ", " << source_back_mapping_to_string(instruction.source_back_mapping());
    ostr << " ]";
    return ostr;
}


static bool save_instructions(std::ostream& ostr, std::vector<Instruction> const& instructions, std::string const& shift)
{
    for (std::size_t i = 0U; i < instructions.size(); ++i)
    {
        if (i != 0U)
            ostr << ',' << DbgLine{i - 1ULL} << '\n';
        auto const& instruction = instructions.at(i);
        ostr << shift << instruction;
    }
    if (!instructions.empty())
        ostr << DbgLine{instructions.size() - 1ULL};
    return !instructions.empty();
}


static bool save_block_successors(std::ostream& ostr, std::vector<std::uint32_t> const& successors)
{
    for (std::size_t i = 0U; i < successors.size(); ++i)
    {
        if (i != 0U)
            ostr << ", ";
        ostr << successors.at(i);
    }
    return !successors.empty();
}


static bool save_basic_blocks(std::ostream& ostr, std::vector<BasicBlock> const& blocks, std::string const& shift)
{
    for (std::size_t i = 0U; i < blocks.size(); ++i)
    {
        if (i != 0U)
            ostr << ",\n";
        auto const& block = blocks.at(i);
        ostr << shift << '{' << DbgLine{i} << '\n'
             << shift << "  \"instructions\": [\n";
        if (save_instructions(ostr, block.instructions(), shift + "    ")) ostr << '\n';
        ostr << shift << "  ],\n"
             << shift << "  \"successors\": [ ";
        if (save_block_successors(ostr, block.successors())) ostr << ' ';
        ostr << "]\n"
             << shift << "}";
    }
    return !blocks.empty();
}


static bool save_functions(std::ostream& ostr, std::vector<Function> const& functions)
{
    for (std::size_t i = 0U; i < functions.size(); ++i)
    {
        if (i != 0U)
            ostr << ",\n";
        auto const& function = functions.at(i);
        ostr << "  {" << DbgLine{i} << '\n'
             << "    \"name\": [ \"" << function.name() << "\", " 
                                     << source_back_mapping_to_string(function.source_back_mapping())
                                     << " ],\n"
             << "    \"parameters\": [\n";
        if (save_variables(ostr, function.parameters(), "      ")) ostr << '\n';
        ostr << "    ],\n"
             << "    \"locals\": [\n";
        if (save_variables(ostr, function.local_variables(), "      ")) ostr << '\n';
        ostr << "    ],\n"
             << "    \"basic_blocks\": [\n";
        if (save_basic_blocks(ostr, function.basic_blocks(), "      ")) ostr << '\n';
        ostr << "    ]\n"
             << "  }";
    }
    return !functions.empty();
}


static bool save_external_variables(std::ostream& ostr, std::vector<std::pair<std::uint32_t, std::string> > const& external_variables)
{
    for (std::size_t i = 0U; i < external_variables.size(); ++i)
    {
        if (i != 0U)
            ostr << ",\n";
        auto const& external_variable = external_variables.at(i);
        ostr << "  [ " << external_variable.first << ", \"" << external_variable.second << "\" ]";
    }
    return !external_variables.empty();
}


static bool save_external_functions(std::ostream& ostr, std::vector<std::uint32_t> const& external_functions)
{
    for (std::size_t i = 0U; i < external_functions.size(); ++i)
    {
        if (i != 0U)
            ostr << ",\n";
        ostr << "  " << external_functions.at(i);
    }
    return !external_functions.empty();
}


std::ostream& operator<<(std::ostream& ostr, Program const& program)
{
    ostr << "{\n";
    ostr << "\"magic\": \"sala\",\n";
    ostr << "\"version\": \"" << program.version() << "\",\n";
    ostr << "\"system\": \"" << program.system() << "\",\n";
    ostr << "\"num_cpu_bits\": " << program.num_cpu_bits() << ",\n";
    ostr << "\"name\": \"" << program.name() << "\",\n";
    ostr << "\"entry_function\": " << program.entry_function() << ",\n";
    ostr << "\"constants\": [\n";
    if (save_constants(ostr, program.constants())) ostr << '\n';
    ostr << "],\n";
    ostr << "\"static\": [\n";
    if (save_variables(ostr, program.static_variables(), "  ")) ostr << '\n';
    ostr << "],\n";
    ostr << "\"functions\": [\n";
    if (save_functions(ostr, program.functions())) ostr << '\n';
    ostr << "],\n";
    ostr << "\"external_variables\": [\n";
    if (save_external_variables(ostr, program.external_variables())) ostr << '\n';
    ostr << "],\n";
    ostr << "\"external_functions\": [\n";
    if (save_external_functions(ostr, program.external_functions())) ostr << '\n';
    ostr << "]\n";
    ostr << "}\n";
    return ostr;
}


}
