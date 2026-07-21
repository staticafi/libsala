#ifndef SALA_STREAMING_UTILS_HPP_INCLUDED
#   define SALA_STREAMING_UTILS_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/opcode_to_string.hpp>
#   include <string>
#   include <vector>
#   include <cstdint>

namespace sala {


std::string instruction_opcode_to_string(Instruction::Opcode const opcode);
std::string instruction_modifier_to_string(Instruction::Modifier const modifier);
std::string instruction_descriptor_to_string(Instruction::Descriptor const descriptor);
std::string source_back_mapping_to_string(SourceBackMapping const& back_mapping);
std::string bytes_to_hex_string(std::vector<std::uint8_t> const& bytes);


}

#endif

