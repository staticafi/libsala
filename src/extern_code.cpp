#include <sala/extern_code.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <cmath>

namespace sala {


template<typename T>
static T __llvm_intrinsic__ctlz_impl(T const value)
{
    T constexpr n = (T)(8U * sizeof(T));
    T i = (T)0;
    T bit_check{ (T)(1 << (sizeof(T) - 1)) };
    while ((value & bit_check) == 0 && i != n)
    {
        ++i;
        bit_check >>= 1;
    }
    return i;
}

template<typename T>
static T __llvm_intrinsic__ctpop_impl(T const value)
{
    T sum{ (T)0 };
    for (T i = (T)0; i != (T)(8U * sizeof(T)); ++i)
        if ((value & (1 << i)) != 0)
            ++sum;
    return sum;
}

template<typename T>
static bool __llvm_intrinsic__is_fpclass(T const value, std::int32_t const bit_mask)
{
    for (std::int32_t  bit = 0U; bit != 10U; ++bit)
        if ((bit_mask & (1U << bit)) != 0U)
            switch (bit)
            {
                case 0:
                case 1:
                    if (std::isnan(value))
                        return true;
                    break;
                case 2:
                    if (!std::isnan(value) && value == -std::numeric_limits<T>::infinity())
                        return true;
                    break;
                case 3:
                    if (!std::isnan(value) && std::isnormal(value) && value < (T)0)
                        return true;
                    break;
                case 4:
                    if (!std::isnan(value) && !std::isnormal(value) && value < (T)0)
                        return true;
                    break;
                case 5:
                    if (!std::isnan(value) && value == (T)0 && std::signbit(value))
                        return true;
                    break;
                case 6:
                    if (!std::isnan(value) && value == (T)0 && !std::signbit(value))
                        return true;
                    break;
                case 7:
                    if (!std::isnan(value) && !std::isnormal(value) && value > (T)0)
                        return true;
                    break;
                case 8:
                    if (!std::isnan(value) && std::isnormal(value) && value > (T)0)
                        return true;
                    break;
                case 9:
                    return !std::isnan(value) && value == std::numeric_limits<T>::infinity();
                        return true;
                    break;
                default:
                    UNREACHABLE();
                    return false;
            }
    return false;
}


template<typename T>
static void  __llvm_intrinsic__abs_impl(MemPtr const  dst_ptr, MemBlock const&  block1, bool const  ret_poison)
{
    T const  value{ block1.read<T>() };
    if (value == std::numeric_limits<T>::min())
    {
        if (ret_poison == false)
            *(T*)dst_ptr = std::numeric_limits<T>::min();
        else
            *(T*)dst_ptr = std::numeric_limits<T>::max(); // Here should be returned LLVM's "poison" value.
    }
    else
        *(T*)dst_ptr = std::abs(value);
}


template<typename T>
void __llvm_intrinsic__ptrmask_impl(MemPtr const  dst_ptr, MemPtr const&  src_ptr, T const  mask)
{
    // TODO!
}


enum OP_WITH_OVERFLOW { OWO_ADD, OWO_SUB, OWO_MUL };
template<typename T>
static void __llvm_intrinsic__operation_with_overflow(std::vector<MemBlock> const& parameters, OP_WITH_OVERFLOW const owo)
{
    T const a{ parameters.at(1).read<T>() };
    T const b{ parameters.at(2).read<T>() };
    T  result;
    bool  flag;
    switch (owo)
    {
        case OWO_ADD:
            flag = __builtin_add_overflow(a,b,&result);
            break;
        case OWO_SUB:
            flag = __builtin_sub_overflow(a,b,&result);
            break;
        case OWO_MUL:
            flag = __builtin_mul_overflow(a,b,&result);
            break;
        default:
            UNREACHABLE();
            break;
    }
    flag = __builtin_add_overflow(a,b,&result);
    auto const dst_ptr{ parameters.front().read<MemPtr>() };
    *(T*)dst_ptr = result;
    ((std::uint8_t*)dst_ptr)[sizeof(T)] = flag ? 1 : 0;
}


ExternCode::ExternCode(ExecState* const state)
    : state_{ state }
    , code_{}
{
    REGISTER_EXTERN_CODE(exit, this->std_exit() );
    REGISTER_EXTERN_CODE(atexit, this->std_atexit() );
    REGISTER_EXTERN_CODE(abort, this->std_abort("abort") );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__bswap_8, this->__llvm_intrinsic__bswap(8ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__bswap_16, this->__llvm_intrinsic__bswap(16ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__bswap_32, this->__llvm_intrinsic__bswap(32ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__bswap_64, this->__llvm_intrinsic__bswap(64ULL) );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctlz_8, this->__llvm_intrinsic__ctlz_8() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctlz_16, this->__llvm_intrinsic__ctlz_16() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctlz_32, this->__llvm_intrinsic__ctlz_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctlz_64, this->__llvm_intrinsic__ctlz_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctpop_8, this->__llvm_intrinsic__ctpop_8() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctpop_16, this->__llvm_intrinsic__ctpop_16() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctpop_32, this->__llvm_intrinsic__ctpop_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ctpop_64, this->__llvm_intrinsic__ctpop_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__trunc_32, this->__llvm_intrinsic__trunc_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__trunc_64, this->__llvm_intrinsic__trunc_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ceil_32, this->__llvm_intrinsic__ceil_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ceil_64, this->__llvm_intrinsic__ceil_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__floor_32, this->__llvm_intrinsic__floor_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__floor_64, this->__llvm_intrinsic__floor_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__round_32, this->__llvm_intrinsic__round_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__round_64, this->__llvm_intrinsic__round_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__rint_32, this->__llvm_intrinsic__rint_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__rint_64, this->__llvm_intrinsic__rint_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__abs_8, this->__llvm_intrinsic__abs_8() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__abs_16, this->__llvm_intrinsic__abs_16() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__abs_32, this->__llvm_intrinsic__abs_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__abs_64, this->__llvm_intrinsic__abs_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__maxnum_32, this->__llvm_intrinsic__maxnum_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__maxnum_64, this->__llvm_intrinsic__maxnum_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__minnum_32, this->__llvm_intrinsic__minnum_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__minnum_64, this->__llvm_intrinsic__minnum_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__copysign_32, this->__llvm_intrinsic__copysign_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__copysign_64, this->__llvm_intrinsic__copysign_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__is_fpclass_32, this->__llvm_intrinsic__is_fpclass_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__is_fpclass_64, this->__llvm_intrinsic__is_fpclass_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ptrmask_32, this->__llvm_intrinsic__ptrmask_32() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ptrmask_64, this->__llvm_intrinsic__ptrmask_64() );
    REGISTER_EXTERN_CODE(__llvm_intrinsic__sadd_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::int16_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__sadd_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::int32_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__sadd_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::int64_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__uadd_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::uint16_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__uadd_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::uint32_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__uadd_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::uint64_t>(this->parameters(), OWO_ADD));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ssub_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::int16_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ssub_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::int32_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__ssub_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::int64_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__usub_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::uint16_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__usub_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::uint32_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__usub_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::uint64_t>(this->parameters(), OWO_SUB));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__smul_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::int16_t>(this->parameters(), OWO_MUL));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__smul_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::int32_t>(this->parameters(), OWO_MUL));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__smul_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::int64_t>(this->parameters(), OWO_MUL));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__umul_with_overflow_16, __llvm_intrinsic__operation_with_overflow<std::uint16_t>(this->parameters(), OWO_MUL));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__umul_with_overflow_32, __llvm_intrinsic__operation_with_overflow<std::uint32_t>(this->parameters(), OWO_MUL));
    REGISTER_EXTERN_CODE(__llvm_intrinsic__umul_with_overflow_64, __llvm_intrinsic__operation_with_overflow<std::uint64_t>(this->parameters(), OWO_MUL));
    // POSIX:
    REGISTER_EXTERN_CODE(__assert_fail, this->std_abort("__assert_fail") );
}


void ExternCode::register_code(std::string const& function_name, std::function<void()> const& code)
{
    code_.insert_or_assign(function_name, code);
}


void ExternCode::call_code_of_current_function_if_registered_external()
{
    if (function().is_external())
    {
        auto it = code_.find(function().name());
        if (it != code_.end())
            it->second();
        else if (!function().name().starts_with("__sbt_fizzer_"))
            state().insert_warning(
                    state().current_location_message() + ": Called unregistered external function '" + function().name() + "'."
                    );
    }
}


Instruction const* ExternCode::get_call_instruction() const
{
    if (state().stack_segment().size() < 2ULL)
        return nullptr;
    auto const& callee_stack_record{ state().stack_segment().at(state().stack_segment().size() - 2ULL) };
    return &program().functions().at(callee_stack_record.function_index())
                     .basic_blocks().at(callee_stack_record.ip().block())
                     .instructions().at(callee_stack_record.ip().instr());
}


void ExternCode::std_exit()
{
    int const exit_code{ parameters().front().read<int>() };

    state().set_stage(ExecState::Stage::TERMINATING);
    state().set_termination(
        ExecState::Termination::NORMAL,
        "test_interpreter[extern_code]",
        "Called exit(" + std::to_string(exit_code) + ").",
        get_call_instruction()
        );
    state().set_exit_code(exit_code);

    state().set_stack_exit_depth(state().stack_segment().size());
}


void ExternCode::std_atexit()
{
    MemPtr const func_ptr{ parameters().back().read<MemPtr>() };
    auto const it = state().functions_at_addresses().find(func_ptr);
    if (it == state().functions_at_addresses().end())
        state().set_termination(
            ExecState::Termination::ERROR,
            "test_interpreter[extern_code]",
            "Called atexit() with an invalid pointer. No function was pushed.",
            get_call_instruction()
            );
    else
        state().push_atexit_function(it->second);
}


void ExternCode::std_abort(std::string const& func_name)
{
    state().set_stage(ExecState::Stage::FINISHED);
    state().set_termination(
        ExecState::Termination::ERROR,
        "test_interpreter[extern_code]",
        "Called " + func_name + "().",
        get_call_instruction());
    state().set_exit_code(0);
}


void ExternCode::__llvm_intrinsic__bswap(std::size_t const num_bytes)
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    auto const src_ptr{ parameters().back().start() };
    for (std::size_t i = 0ULL; i != num_bytes; ++i)
        *(dst_ptr + num_bytes - (i + 1ULL)) = *(src_ptr + i);
}


void ExternCode::__llvm_intrinsic__ctlz_8()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint8_t*)dst_ptr = __llvm_intrinsic__ctlz_impl(parameters().at(1).read<std::uint8_t>());
}


void ExternCode::__llvm_intrinsic__ctlz_16()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint16_t*)dst_ptr = __llvm_intrinsic__ctlz_impl(parameters().at(1).read<std::uint16_t>());
}


void ExternCode::__llvm_intrinsic__ctlz_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint32_t*)dst_ptr = __llvm_intrinsic__ctlz_impl(parameters().at(1).read<std::uint32_t>());
}


void ExternCode::__llvm_intrinsic__ctlz_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint64_t*)dst_ptr = __llvm_intrinsic__ctlz_impl(parameters().at(1).read<std::uint64_t>());
}


void ExternCode::__llvm_intrinsic__ctpop_8()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint8_t*)dst_ptr = __llvm_intrinsic__ctpop_impl(parameters().at(1).read<std::uint8_t>());
}


void ExternCode::__llvm_intrinsic__ctpop_16()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint16_t*)dst_ptr = __llvm_intrinsic__ctpop_impl(parameters().at(1).read<std::uint16_t>());
}


void ExternCode::__llvm_intrinsic__ctpop_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint32_t*)dst_ptr = __llvm_intrinsic__ctpop_impl(parameters().at(1).read<std::uint32_t>());
}


void ExternCode::__llvm_intrinsic__ctpop_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint64_t*)dst_ptr = __llvm_intrinsic__ctpop_impl(parameters().at(1).read<std::uint64_t>());
}


void ExternCode::__llvm_intrinsic__trunc_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::trunc(parameters().at(1).read<float>());
}


void ExternCode::__llvm_intrinsic__trunc_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::trunc(parameters().at(1).read<double>());
}


void ExternCode::__llvm_intrinsic__ceil_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::ceil(parameters().at(1).read<float>());
}


void ExternCode::__llvm_intrinsic__ceil_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::ceil(parameters().at(1).read<double>());
}


void ExternCode::__llvm_intrinsic__floor_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::floor(parameters().at(1).read<float>());
}


void ExternCode::__llvm_intrinsic__floor_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::floor(parameters().at(1).read<double>());
}


void ExternCode::__llvm_intrinsic__round_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::round(parameters().at(1).read<float>());
}


void ExternCode::__llvm_intrinsic__round_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::round(parameters().at(1).read<double>());
}


void ExternCode::__llvm_intrinsic__rint_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::rint(parameters().at(1).read<float>());
}


void ExternCode::__llvm_intrinsic__rint_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::rint(parameters().at(1).read<double>());
}


void ExternCode::__llvm_intrinsic__abs_8()
{
    __llvm_intrinsic__abs_impl<std::int8_t>(parameters().front().read<MemPtr>(), parameters().at(1), parameters().at(2).read<std::uint8_t>() != 0U);
}


void ExternCode::__llvm_intrinsic__abs_16()
{
    __llvm_intrinsic__abs_impl<std::int16_t>(parameters().front().read<MemPtr>(), parameters().at(1), parameters().at(2).read<std::uint8_t>() != 0U);
}


void ExternCode::__llvm_intrinsic__abs_32()
{
    __llvm_intrinsic__abs_impl<std::int32_t>(parameters().front().read<MemPtr>(), parameters().at(1), parameters().at(2).read<std::uint8_t>() != 0U);
}


void ExternCode::__llvm_intrinsic__abs_64()
{
    __llvm_intrinsic__abs_impl<std::int64_t>(parameters().front().read<MemPtr>(), parameters().at(1), parameters().at(2).read<std::uint8_t>() != 0U);
}


void ExternCode::__llvm_intrinsic__maxnum_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::fmaxf(parameters().at(1).read<float>(),parameters().at(2).read<float>());
}


void ExternCode::__llvm_intrinsic__maxnum_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::fmax(parameters().at(1).read<double>(), parameters().at(2).read<double>());
}


void ExternCode::__llvm_intrinsic__minnum_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::fminf(parameters().at(1).read<float>(),parameters().at(2).read<float>());
}


void ExternCode::__llvm_intrinsic__minnum_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::fmin(parameters().at(1).read<double>(), parameters().at(2).read<double>());
}


void ExternCode::__llvm_intrinsic__copysign_32()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(float*)dst_ptr = std::copysignf(parameters().at(1).read<float>(),parameters().at(2).read<float>());
}


void ExternCode::__llvm_intrinsic__copysign_64()
{
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(double*)dst_ptr = std::copysign(parameters().at(1).read<double>(), parameters().at(2).read<double>());
}


void ExternCode::__llvm_intrinsic__is_fpclass_32()
{
    bool const result{ __llvm_intrinsic__is_fpclass(parameters().at(1).read<float>(), parameters().at(2).read<std::int32_t>()) };
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint8_t*)dst_ptr = result ? 1 : 0;
}


void ExternCode::__llvm_intrinsic__is_fpclass_64()
{
    bool const result{ __llvm_intrinsic__is_fpclass(parameters().at(1).read<double>(), parameters().at(2).read<std::int32_t>()) };
    auto const dst_ptr{ parameters().front().read<MemPtr>() };
    *(std::uint8_t*)dst_ptr = result ? 1 : 0;
}


void ExternCode::__llvm_intrinsic__ptrmask_32()
{
    __llvm_intrinsic__ptrmask_impl<std::int32_t>(
            parameters().front().read<MemPtr>(),
            parameters().at(1).read<MemPtr>(),
            parameters().at(2).read<std::uint32_t>()
            );
}


void ExternCode::__llvm_intrinsic__ptrmask_64()
{
    __llvm_intrinsic__ptrmask_impl<std::int64_t>(
            parameters().front().read<MemPtr>(),
            parameters().at(1).read<MemPtr>(),
            parameters().at(2).read<std::uint64_t>()
            );
}


}
