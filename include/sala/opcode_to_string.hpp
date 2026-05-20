#ifndef SALA_OPCODE_TO_STRING_HPP_INCLUDED
#   define SALA_OPCODE_TO_STRING_HPP_INCLUDED

#   include <string>
#   include <sala/program.hpp>

namespace sala {

std::string instruction_opcode_to_string(Instruction::Opcode const opcode);

}

#endif

