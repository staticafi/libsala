#include <sala/instr_switch.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace sala {


bool InstrSwitch::do_instruction_switch()
{
    switch (instruction().opcode())
    {
        case Instruction::Opcode::NOP: do_nop(); return false;

        case Instruction::Opcode::HALT: do_halt(); return false;

        case Instruction::Opcode::ADDRESS: do_address(); return false;
        case Instruction::Opcode::LOAD: do_load(); return false;
        case Instruction::Opcode::STORE: do_store(); return false;

        case Instruction::Opcode::COPY:
            switch (operands().front()->count())
            {
            case 1ULL: do_copy_8(); return false;
            case 2ULL: do_copy_16(); return false;
            case 4ULL: do_copy_32(); return false;
            case 8ULL: do_copy_64(); return false;
            default: do_copy(); return false;
            }

        case Instruction::Opcode::MEMCPY: do_memcpy(); return false;
        case Instruction::Opcode::MEMMOVE: do_memmove(); return false;
        case Instruction::Opcode::MEMSET: do_memset(); return false;
        case Instruction::Opcode::MOVEPTR: do_moveptr(); return false;

        case Instruction::Opcode::ALLOCA: do_alloca(); return false;
        case Instruction::Opcode::STACKSAVE: do_stacksave(); return false;
        case Instruction::Opcode::STACKRESTORE: do_stackrestore(); return false;
        case Instruction::Opcode::MALLOC: do_malloc(); return false;
        case Instruction::Opcode::FREE: do_free(); return false;

        case Instruction::Opcode::ADD:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_add_s8(); return false;
                case 2ULL: do_add_s16(); return false;
                case 4ULL: do_add_s32(); return false;
                case 8ULL: do_add_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_add_u8(); return false;
                case 2ULL: do_add_u16(); return false;
                case 4ULL: do_add_u32(); return false;
                case 8ULL: do_add_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().front()->count())
                {
                case 4ULL: do_add_f32(); return false;
                case 8ULL: do_add_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }
    
        case Instruction::Opcode::SUB:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_sub_s8(); return false;
                case 2ULL: do_sub_s16(); return false;
                case 4ULL: do_sub_s32(); return false;
                case 8ULL: do_sub_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_sub_u8(); return false;
                case 2ULL: do_sub_u16(); return false;
                case 4ULL: do_sub_u32(); return false;
                case 8ULL: do_sub_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().front()->count())
                {
                case 4ULL: do_sub_f32(); return false;
                case 8ULL: do_sub_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::MUL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_mul_s8(); return false;
                case 2ULL: do_mul_s16(); return false;
                case 4ULL: do_mul_s32(); return false;
                case 8ULL: do_mul_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_mul_u8(); return false;
                case 2ULL: do_mul_u16(); return false;
                case 4ULL: do_mul_u32(); return false;
                case 8ULL: do_mul_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().front()->count())
                {
                case 4ULL: do_mul_f32(); return false;
                case 8ULL: do_mul_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::DIV:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_div_s8(); return false;
                case 2ULL: do_div_s16(); return false;
                case 4ULL: do_div_s32(); return false;
                case 8ULL: do_div_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_div_u8(); return false;
                case 2ULL: do_div_u16(); return false;
                case 4ULL: do_div_u32(); return false;
                case 8ULL: do_div_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().front()->count())
                {
                case 4ULL: do_div_f32(); return false;
                case 8ULL: do_div_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::REM:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_rem_s8(); return false;
                case 2ULL: do_rem_s16(); return false;
                case 4ULL: do_rem_s32(); return false;
                case 8ULL: do_rem_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_rem_u8(); return false;
                case 2ULL: do_rem_u16(); return false;
                case 4ULL: do_rem_u32(); return false;
                case 8ULL: do_rem_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::AND:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::NONE:
                switch (operands().front()->count())
                {
                case 1ULL: do_and_8(); return false;
                case 2ULL: do_and_16(); return false;
                case 4ULL: do_and_32(); return false;
                case 8ULL: do_and_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::OR:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::NONE:
                switch (operands().front()->count())
                {
                case 1ULL: do_or_8(); return false;
                case 2ULL: do_or_16(); return false;
                case 4ULL: do_or_32(); return false;
                case 8ULL: do_or_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::XOR:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::NONE:
                switch (operands().front()->count())
                {
                case 1ULL: do_xor_8(); return false;
                case 2ULL: do_xor_16(); return false;
                case 4ULL: do_xor_32(); return false;
                case 8ULL: do_xor_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::SHL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::NONE:
                switch (operands().front()->count())
                {
                case 1ULL: do_shl_8(); return false;
                case 2ULL: do_shl_16(); return false;
                case 4ULL: do_shl_32(); return false;
                case 8ULL: do_shl_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::SHR:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_shr_s8(); return false;
                case 2ULL: do_shr_s16(); return false;
                case 4ULL: do_shr_s32(); return false;
                case 8ULL: do_shr_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_shr_u8(); return false;
                case 2ULL: do_shr_u16(); return false;
                case 4ULL: do_shr_u32(); return false;
                case 8ULL: do_shr_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::NEG:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::FLOATING:
                switch (operands().front()->count())
                {
                case 4ULL: do_neg_f32(); return false;
                case 8ULL: do_neg_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::EXTEND:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 1ULL:
                    switch (operands().front()->count())
                    {
                    case 2ULL: do_extend_s8_s16(); return false;
                    case 4ULL: do_extend_s8_s32(); return false;
                    case 8ULL: do_extend_s8_s64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 2ULL:
                    switch (operands().front()->count())
                    {
                    case 4ULL: do_extend_s16_s32(); return false;
                    case 8ULL: do_extend_s16_s64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 8ULL: do_extend_s32_s64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL:
                    switch (operands().front()->count())
                    {
                    case 2ULL: do_extend_u8_u16(); return false;
                    case 4ULL: do_extend_u8_u32(); return false;
                    case 8ULL: do_extend_u8_u64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 2ULL:
                    switch (operands().front()->count())
                    {
                    case 4ULL: do_extend_u16_u32(); return false;
                    case 8ULL: do_extend_u16_u64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 8ULL: do_extend_u32_u64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 8ULL: do_extend_f32_f64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::TRUNCATE:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 2ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_truncate_u16_u8(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_truncate_u32_u8(); return false;
                    case 2ULL: do_truncate_u32_u16(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 8ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_truncate_u64_u8(); return false;
                    case 2ULL: do_truncate_u64_u16(); return false;
                    case 4ULL: do_truncate_u64_u32(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 8ULL:
                    switch (operands().front()->count())
                    {
                    case 4ULL: do_truncate_f64_f32(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::F2I:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_f2i_f32_s8(); return false;
                    case 2ULL: do_f2i_f32_s16(); return false;
                    case 4ULL: do_f2i_f32_s32(); return false;
                    case 8ULL: do_f2i_f32_s64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 8ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_f2i_f64_s8(); return false;
                    case 2ULL: do_f2i_f64_s16(); return false;
                    case 4ULL: do_f2i_f64_s32(); return false;
                    case 8ULL: do_f2i_f64_s64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 4ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_f2i_f32_u8(); return false;
                    case 2ULL: do_f2i_f32_u16(); return false;
                    case 4ULL: do_f2i_f32_u32(); return false;
                    case 8ULL: do_f2i_f32_u64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 8ULL:
                    switch (operands().front()->count())
                    {
                    case 1ULL: do_f2i_f64_u8(); return false;
                    case 2ULL: do_f2i_f64_u16(); return false;
                    case 4ULL: do_f2i_f64_u32(); return false;
                    case 8ULL: do_f2i_f64_u64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::I2F:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().front()->count())
                {
                case 4ULL:
                    switch (operands().back()->count())
                    {
                    case 1ULL: do_i2f_s8_f32(); return false;
                    case 2ULL: do_i2f_s16_f32(); return false;
                    case 4ULL: do_i2f_s32_f32(); return false;
                    case 8ULL: do_i2f_s64_f32(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 8ULL:
                    switch (operands().back()->count())
                    {
                    case 1ULL: do_i2f_s8_f64(); return false;
                    case 2ULL: do_i2f_s16_f64(); return false;
                    case 4ULL: do_i2f_s32_f64(); return false;
                    case 8ULL: do_i2f_s64_f64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 4ULL:
                    switch (operands().back()->count())
                    {
                    case 1ULL: do_i2f_u8_f32(); return false;
                    case 2ULL: do_i2f_u16_f32(); return false;
                    case 4ULL: do_i2f_u32_f32(); return false;
                    case 8ULL: do_i2f_u64_f32(); return false;
                    default: UNREACHABLE(); return false;
                    }
                case 8ULL:
                    switch (operands().back()->count())
                    {
                    case 1ULL: do_i2f_u8_f64(); return false;
                    case 2ULL: do_i2f_u16_f64(); return false;
                    case 4ULL: do_i2f_u32_f64(); return false;
                    case 8ULL: do_i2f_u64_f64(); return false;
                    default: UNREACHABLE(); return false;
                    }
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::P2I:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::UNSIGNED:
                switch (operands().front()->count())
                {
                case 1ULL: do_p2i_8(); return false;
                case 2ULL: do_p2i_16(); return false;
                case 4ULL: do_p2i_32(); return false;
                case 8ULL: do_p2i_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::I2P:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_i2p_8(); return false;
                case 2ULL: do_i2p_16(); return false;
                case 4ULL: do_i2p_32(); return false;
                case 8ULL: do_i2p_64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::LESS:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_less_s8(); return false;
                case 2ULL: do_less_s16(); return false;
                case 4ULL: do_less_s32(); return false;
                case 8ULL: do_less_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_less_u8(); return false;
                case 2ULL: do_less_u16(); return false;
                case 4ULL: do_less_u32(); return false;
                case 8ULL: do_less_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_less_f32(); return false;
                case 8ULL: do_less_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_less_w32(); return false;
                case 8ULL: do_less_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::LESS_EQUAL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_less_equal_s8(); return false;
                case 2ULL: do_less_equal_s16(); return false;
                case 4ULL: do_less_equal_s32(); return false;
                case 8ULL: do_less_equal_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_less_equal_u8(); return false;
                case 2ULL: do_less_equal_u16(); return false;
                case 4ULL: do_less_equal_u32(); return false;
                case 8ULL: do_less_equal_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_less_equal_f32(); return false;
                case 8ULL: do_less_equal_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_less_equal_w32(); return false;
                case 8ULL: do_less_equal_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::GREATER:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_greater_s8(); return false;
                case 2ULL: do_greater_s16(); return false;
                case 4ULL: do_greater_s32(); return false;
                case 8ULL: do_greater_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_greater_u8(); return false;
                case 2ULL: do_greater_u16(); return false;
                case 4ULL: do_greater_u32(); return false;
                case 8ULL: do_greater_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_greater_f32(); return false;
                case 8ULL: do_greater_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_greater_w32(); return false;
                case 8ULL: do_greater_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::GREATER_EQUAL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::SIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_greater_equal_s8(); return false;
                case 2ULL: do_greater_equal_s16(); return false;
                case 4ULL: do_greater_equal_s32(); return false;
                case 8ULL: do_greater_equal_s64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_greater_equal_u8(); return false;
                case 2ULL: do_greater_equal_u16(); return false;
                case 4ULL: do_greater_equal_u32(); return false;
                case 8ULL: do_greater_equal_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_greater_equal_f32(); return false;
                case 8ULL: do_greater_equal_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_greater_equal_w32(); return false;
                case 8ULL: do_greater_equal_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::EQUAL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_equal_u8(); return false;
                case 2ULL: do_equal_u16(); return false;
                case 4ULL: do_equal_u32(); return false;
                case 8ULL: do_equal_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_equal_f32(); return false;
                case 8ULL: do_equal_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_equal_w32(); return false;
                case 8ULL: do_equal_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::UNEQUAL:
            switch (instruction().modifier())
            {
            case Instruction::Modifier::UNSIGNED:
                switch (operands().back()->count())
                {
                case 1ULL: do_unequal_u8(); return false;
                case 2ULL: do_unequal_u16(); return false;
                case 4ULL: do_unequal_u32(); return false;
                case 8ULL: do_unequal_u64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING:
                switch (operands().back()->count())
                {
                case 4ULL: do_unequal_f32(); return false;
                case 8ULL: do_unequal_f64(); return false;
                default: UNREACHABLE(); return false;
                }
            case Instruction::Modifier::FLOATING_UNORDERED:
                switch (operands().back()->count())
                {
                case 4ULL: do_unequal_w32(); return false;
                case 8ULL: do_unequal_w64(); return false;
                default: UNREACHABLE(); return false;
                }
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::ISNAN:
            switch (operands().back()->count())
            {
            case 4ULL: do_isnan_w32(); return false;
            case 8ULL: do_isnan_w64(); return false;
            default: UNREACHABLE(); return false;
            }

        case Instruction::Opcode::VA_START: do_va_start(); return false;
        case Instruction::Opcode::VA_END: do_va_end(); return false;
        case Instruction::Opcode::VA_ARG: do_va_arg(); return false;

        case Instruction::Opcode::JUMP: do_jump(); return true;
        case Instruction::Opcode::BRANCH: do_branch(); return true;
        case Instruction::Opcode::CALL: do_call(); return true;
        case Instruction::Opcode::RET: do_ret(); return true;

        default: UNREACHABLE(); return false;
    }
}


}
