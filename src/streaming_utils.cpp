#include <sala/streaming_utils.hpp>
#include <utility/invariants.hpp>
#include <sstream>
#include <iomanip>

namespace sala {


std::string instruction_modifier_to_string(Instruction::Modifier const modifier)
{
    switch (modifier)
    {
        case Instruction::Modifier::NONE: return "n"; break;
        case Instruction::Modifier::SIGNED: return "s"; break;
        case Instruction::Modifier::UNSIGNED: return "u"; break;
        case Instruction::Modifier::FLOATING: return "f"; break;
        case Instruction::Modifier::FLOATING_UNORDERED: return "w"; break;
        default: UNREACHABLE(); break;
    }
}


std::string instruction_descriptor_to_string(Instruction::Descriptor const descriptor)
{
    switch (descriptor)
    {
        case Instruction::Descriptor::STATIC: return "s"; break;
        case Instruction::Descriptor::LOCAL: return "l"; break;
        case Instruction::Descriptor::PARAMETER: return "p"; break;
        case Instruction::Descriptor::CONSTANT: return "c"; break;
        case Instruction::Descriptor::FUNCTION: return "f"; break;
        default: UNREACHABLE(); break;
    }
}


std::string source_back_mapping_to_string(SourceBackMapping const& back_mapping)
{
    std::stringstream sstr;
    sstr << "[" << back_mapping.line << "," << back_mapping.column << "]";
    return sstr.str();
}


std::string bytes_to_hex_string(std::vector<std::uint8_t> const& bytes)
{
    std::stringstream sstr;
    for (auto byte : bytes)
        sstr << std::setfill('0') << std::setw(2) << std::hex << (std::uint32_t)byte;
    return sstr.str();
}


}
