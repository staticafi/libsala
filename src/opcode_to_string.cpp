#include <sala/opcode_to_string.hpp>
#include <sala/program.hpp>
#include <utility/invariants.hpp>

namespace sala {

std::string instruction_opcode_to_string(Instruction::Opcode const opcode)
{
    switch (opcode)
    {
        case Instruction::Opcode::__INVALID__: return "__INVALID__"; break;
        case Instruction::Opcode::NOP: return "NOP"; break;
        case Instruction::Opcode::HALT: return "HALT"; break;
        case Instruction::Opcode::ADDRESS: return "ADDRESS"; break;
        case Instruction::Opcode::LOAD: return "LOAD"; break;
        case Instruction::Opcode::STORE: return "STORE"; break;
        case Instruction::Opcode::COPY: return "COPY"; break;
        case Instruction::Opcode::MEMCPY: return "MEMCPY"; break;
        case Instruction::Opcode::MEMMOVE: return "MEMMOVE"; break;
        case Instruction::Opcode::MEMSET: return "MEMSET"; break;
        case Instruction::Opcode::MOVEPTR: return "MOVEPTR"; break;
        case Instruction::Opcode::ALLOCA: return "ALLOCA"; break;
        case Instruction::Opcode::STACKSAVE: return "STACKSAVE"; break;
        case Instruction::Opcode::STACKRESTORE: return "STACKRESTORE"; break;
        case Instruction::Opcode::MALLOC: return "MALLOC"; break;
        case Instruction::Opcode::FREE: return "FREE"; break;
        case Instruction::Opcode::ADD: return "ADD"; break;
        case Instruction::Opcode::SUB: return "SUB"; break;
        case Instruction::Opcode::MUL: return "MUL"; break;
        case Instruction::Opcode::DIV: return "DIV"; break;
        case Instruction::Opcode::REM: return "REM"; break;
        case Instruction::Opcode::AND: return "AND"; break;
        case Instruction::Opcode::OR: return "OR"; break;
        case Instruction::Opcode::XOR: return "XOR"; break;
        case Instruction::Opcode::SHL: return "SHL"; break;
        case Instruction::Opcode::SHR: return "SHR"; break;
        case Instruction::Opcode::NEG: return "NEG"; break;
        case Instruction::Opcode::EXTEND: return "EXTEND"; break;
        case Instruction::Opcode::TRUNCATE: return "TRUNCATE"; break;
        case Instruction::Opcode::F2I: return "F2I"; break;
        case Instruction::Opcode::I2F: return "I2F"; break;
        case Instruction::Opcode::P2I: return "P2I"; break;
        case Instruction::Opcode::I2P: return "I2P"; break;
        case Instruction::Opcode::LESS: return "LESS"; break;
        case Instruction::Opcode::LESS_EQUAL: return "LESS_EQUAL"; break;
        case Instruction::Opcode::GREATER: return "GREATER"; break;
        case Instruction::Opcode::GREATER_EQUAL: return "GREATER_EQUAL"; break;
        case Instruction::Opcode::EQUAL: return "EQUAL"; break;
        case Instruction::Opcode::UNEQUAL: return "UNEQUAL"; break;
        case Instruction::Opcode::ISNAN: return "ISNAN"; break;
        case Instruction::Opcode::JUMP: return "JUMP"; break;
        case Instruction::Opcode::BRANCH: return "BRANCH"; break;
        case Instruction::Opcode::CALL: return "CALL"; break;
        case Instruction::Opcode::RET: return "RET"; break;
        case Instruction::Opcode::VA_START: return "VA_START"; break;
        case Instruction::Opcode::VA_END: return "VA_END"; break;
        case Instruction::Opcode::VA_ARG: return "VA_ARG"; break;
        case Instruction::Opcode::VA_COPY: return "VA_COPY"; break;
        default: UNREACHABLE(); break;
    }
}

}

