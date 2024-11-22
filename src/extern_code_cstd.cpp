#include <sala/extern_code_cstd.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <cmath>
#include <cfenv>
#include <unistd.h>
#include <getopt.h>

#define REGISTER_CONST_FUNC(FN_NAME, TYPE) \
    REGISTER_EXTERN_CODE(FN_NAME, \
        TYPE const result{ FN_NAME() }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(TYPE)))
#define REGISTER_UNARY_FUNC_IMPL(LL_FN_NAME, FN_NAME, PARAM_TYPE, RET_TYPE) \
    REGISTER_EXTERN_CODE(LL_FN_NAME, \
        RET_TYPE const result{ FN_NAME(parameters().back().read<PARAM_TYPE>()) }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(RET_TYPE)))
#define REGISTER_UNARY_FUNC(FN_NAME, TYPE) REGISTER_UNARY_FUNC_IMPL(FN_NAME, FN_NAME, TYPE, TYPE)
#define REGISTER_BINARY_FUNC_IMPL(LL_FN_NAME, FN_NAME, PARAM1_TYPE, PARAM2_TYPE, RET_TYPE) \
    REGISTER_EXTERN_CODE(LL_FN_NAME, \
        RET_TYPE const result{ FN_NAME(parameters().at(1).read<PARAM1_TYPE>(), parameters().back().read<PARAM2_TYPE>()) }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(RET_TYPE)))
#define REGISTER_BINARY_FUNC(FN_NAME, TYPE) REGISTER_BINARY_FUNC_IMPL(FN_NAME, FN_NAME, TYPE, TYPE, TYPE)
#define REGISTER_TERNARY_FUNC_IMPL(LL_FN_NAME, FN_NAME, PARAM1_TYPE, PARAM2_TYPE, PARAM3_TYPE, RET_TYPE) \
    REGISTER_EXTERN_CODE(LL_FN_NAME, \
        RET_TYPE const result{ FN_NAME(parameters().at(1).read<PARAM1_TYPE>(), parameters().at(2).read<PARAM2_TYPE>(), \
                                       parameters().at(3).read<PARAM3_TYPE>()) \
                                       }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(RET_TYPE)))
#define REGISTER_TERNARY_FUNC(FN_NAME, TYPE) REGISTER_BINARY_FUNC_IMPL(FN_NAME, FN_NAME, TYPE, TYPE, TYPE, TYPE)
#define REGISTER_4ARY_FUNC_IMPL(LL_FN_NAME, FN_NAME, PARAM1_TYPE, PARAM2_TYPE, PARAM3_TYPE, PARAM4_TYPE, RET_TYPE) \
    REGISTER_EXTERN_CODE(LL_FN_NAME, \
        RET_TYPE const result{ FN_NAME(parameters().at(1).read<PARAM1_TYPE>(), parameters().at(2).read<PARAM2_TYPE>(), \
                                       parameters().at(3).read<PARAM3_TYPE>(), parameters().at(4).read<PARAM4_TYPE>()) \
                                       }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(RET_TYPE)))
#define REGISTER_4ARY_FUNC(FN_NAME, TYPE) REGISTER_BINARY_FUNC_IMPL(FN_NAME, FN_NAME, TYPE, TYPE, TYPE, TYPE, TYPE)
#define REGISTER_5ARY_FUNC_IMPL(LL_FN_NAME, FN_NAME, PARAM1_TYPE, PARAM2_TYPE, PARAM3_TYPE, PARAM4_TYPE, PARAM5_TYPE, RET_TYPE) \
    REGISTER_EXTERN_CODE(LL_FN_NAME, \
        RET_TYPE const result{ FN_NAME(parameters().at(1).read<PARAM1_TYPE>(), parameters().at(2).read<PARAM2_TYPE>(), \
                                       parameters().at(3).read<PARAM3_TYPE>(), parameters().at(4).read<PARAM4_TYPE>(), \
                                       parameters().at(5).read<PARAM5_TYPE>()) \
                                       }; \
        std::memcpy(parameters().front().read<sala::MemPtr>(), &result, sizeof(RET_TYPE)))
#define REGISTER_5ARY_FUNC(FN_NAME, TYPE) REGISTER_BINARY_FUNC_IMPL(FN_NAME, FN_NAME, TYPE, TYPE, TYPE, TYPE, TYPE, TYPE)

namespace sala {


ExternCodeCStd::ExternCodeCStd(ExecState* const state, Sanitizer* const sanitizer)
    : ExternCode{ state, sanitizer }
{
    register_math_functions();
    register_string_functions();
    register_fenv_functions();
    register_linux_functions();
}


void ExternCodeCStd::register_math_functions()
{
    REGISTER_UNARY_FUNC(acos, double);
    REGISTER_UNARY_FUNC(acosf, float);
    REGISTER_UNARY_FUNC(acosh, double);
    REGISTER_UNARY_FUNC(acoshf, float);
    REGISTER_UNARY_FUNC(asin, double);
    REGISTER_UNARY_FUNC(asinf, float);
    REGISTER_UNARY_FUNC(asinh, double);
    REGISTER_UNARY_FUNC(asinhf, float);
    REGISTER_UNARY_FUNC(atan, double);
    REGISTER_UNARY_FUNC(atanf, float);
    REGISTER_UNARY_FUNC(atanh, double);
    REGISTER_UNARY_FUNC(atanhf, float);
    REGISTER_UNARY_FUNC(ceil, double);
    REGISTER_UNARY_FUNC(ceilf, float);
    REGISTER_UNARY_FUNC(cos, double);
    REGISTER_UNARY_FUNC(cosf, float);
    REGISTER_UNARY_FUNC(cosh, double);
    REGISTER_UNARY_FUNC(coshf, float);
    REGISTER_UNARY_FUNC(exp, double);
    REGISTER_UNARY_FUNC(expf, float);
    REGISTER_UNARY_FUNC(exp2, double);
    REGISTER_UNARY_FUNC(exp2f, float);
    REGISTER_UNARY_FUNC(fabs, double);
    REGISTER_UNARY_FUNC(fabsf, float);
    REGISTER_UNARY_FUNC(floor, double);
    REGISTER_UNARY_FUNC(floorf, float);
    REGISTER_UNARY_FUNC(log, double);
    REGISTER_UNARY_FUNC(logf, float);
    REGISTER_UNARY_FUNC(log2, double);
    REGISTER_UNARY_FUNC(log2f, float);
    REGISTER_UNARY_FUNC(log10, double);
    REGISTER_UNARY_FUNC(log10f, float);
    REGISTER_UNARY_FUNC(round, double);
    REGISTER_UNARY_FUNC(roundf, float);
    REGISTER_UNARY_FUNC(sin, double);
    REGISTER_UNARY_FUNC(sinf, float);
    REGISTER_UNARY_FUNC(sinh, double);
    REGISTER_UNARY_FUNC(sinhf, float);
    REGISTER_UNARY_FUNC(sqrt, double);
    REGISTER_UNARY_FUNC(sqrtf, float);
    REGISTER_UNARY_FUNC(tan, double);
    REGISTER_UNARY_FUNC(tanf, float);
    REGISTER_UNARY_FUNC(tanh, double);
    REGISTER_UNARY_FUNC(tanhf, float);
    REGISTER_UNARY_FUNC(trunc, double);
    REGISTER_UNARY_FUNC(truncf, float);

    REGISTER_UNARY_FUNC_IMPL(__isinf, __isinf, double, int);
    REGISTER_UNARY_FUNC_IMPL(__isnan, __isnan, double, int);
    REGISTER_UNARY_FUNC_IMPL(__finite, __finite, double, int);
    REGISTER_UNARY_FUNC_IMPL(__signbit, __signbit, double, int);
    REGISTER_UNARY_FUNC_IMPL(__fpclassifyf, __fpclassifyf, float, int);
    REGISTER_UNARY_FUNC_IMPL(__fpclassifyl, __fpclassifyl, long double, int);
    REGISTER_UNARY_FUNC_IMPL(__fpclassify, __fpclassify, double, int);
    REGISTER_UNARY_FUNC_IMPL(__issignaling, __issignaling, double, int);

    REGISTER_BINARY_FUNC(atan2, double);
    REGISTER_BINARY_FUNC(atan2f, float);
    REGISTER_BINARY_FUNC(copysign, double);
    REGISTER_BINARY_FUNC(copysignf, float);
    REGISTER_BINARY_FUNC(fmod, double);
    REGISTER_BINARY_FUNC(fmodf, float);
    REGISTER_BINARY_FUNC(remainder, double);
    REGISTER_BINARY_FUNC(remainderf, float);

    REGISTER_BINARY_FUNC_IMPL(__iseqsig, __iseqsig, double, double, int);
}


void ExternCodeCStd::register_string_functions()
{
    REGISTER_EXTERN_CODE(strlen, this->strlen_impl());
    REGISTER_EXTERN_CODE(strchr, this->strchr_impl());
    REGISTER_EXTERN_CODE(strrchr, this->strrchr_impl());
    REGISTER_EXTERN_CODE(strspn, this->strspn_impl());
    REGISTER_EXTERN_CODE(strcspn, this->strcspn_impl());
    REGISTER_EXTERN_CODE(strpbrk, this->strpbrk_impl());
    REGISTER_EXTERN_CODE(strstr, this->strstr_impl());
    REGISTER_EXTERN_CODE(strtok, this->strtok_impl());
    REGISTER_EXTERN_CODE(strcat, this->strcat_impl());
    REGISTER_EXTERN_CODE(strncat, this->strncat_impl());
    REGISTER_EXTERN_CODE(strcpy, this->strcpy_impl());
    REGISTER_EXTERN_CODE(strncpy, this->strncpy_impl());
    REGISTER_EXTERN_CODE(strcmp, this->strcmp_impl());
    REGISTER_EXTERN_CODE(strncmp, this->strncmp_impl());
}


void ExternCodeCStd::register_fenv_functions()
{
    REGISTER_CONST_FUNC(fegetround, int);
    REGISTER_UNARY_FUNC(fesetround, int);
}


void ExternCodeCStd::register_linux_functions()
{
    REGISTER_EXTERN_CODE(getopt, this->getopt_impl());
    REGISTER_EXTERN_CODE(getopt_long, this->getopt_long_impl());
}


void ExternCodeCStd::crash_execution(std::string const& message)
{
    state().set_stage(ExecState::Stage::FINISHED);
    state().set_termination(
        ExecState::Termination::CRASH,
        "ExternCodeCStd",
        state().make_error_message(message)
        );
    state().set_exit_code(0);
}


void ExternCodeCStd::strlen_impl()
{
    MemPtr const str{ parameters().at(1).read<MemPtr>() };
    if (!sanitizer()->is_c_string_valid(str))
    {
        crash_execution("strlen_impl: Argument is not valid C string.");
        return;
    }
    auto const dst_ptr{ parameters().front().read<std::size_t*>() };
    *dst_ptr = std::strlen((char const*)str);
}


void ExternCodeCStd::strchr_impl()
{
    MemPtr const str{ parameters().at(1).read<MemPtr>() };
    if (!sanitizer()->is_c_string_valid(str))
    {
        crash_execution("strchr_impl: Argument is not valid C string.");
        return;
    }
    auto const chr{ parameters().at(2).read<int>() };
    auto const dst_ptr{ parameters().front().read<char**>() };
    *dst_ptr = std::strchr((char*)str, chr);
}


void ExternCodeCStd::strrchr_impl()
{
    MemPtr const str{ parameters().at(1).read<MemPtr>() };
    if (!sanitizer()->is_c_string_valid(str))
    {
        crash_execution("strrchr_impl: Argument is not valid C string.");
        return;
    }
    auto const chr{ parameters().at(2).read<int>() };
    auto const dst_ptr{ parameters().front().read<char**>() };
    *dst_ptr = std::strrchr((char*)str, chr);
}


void ExternCodeCStd::strspn_impl()
{
    crash_execution("strspn_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strcspn_impl()
{
    crash_execution("strcspn_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strpbrk_impl()
{
    crash_execution("strpbrk_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strstr_impl()
{
    crash_execution("strstr_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strtok_impl()
{
    crash_execution("strtok_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strcat_impl()
{
    crash_execution("strcat_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strncat_impl()
{
    crash_execution("strncat_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strcpy_impl()
{
    crash_execution("strcpy_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strncpy_impl()
{
    crash_execution("strncpy_impl: NOT IMPLEMENTED YET.");
}


void ExternCodeCStd::strcmp_impl()
{
    auto const lhs{ parameters().at(1).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)lhs))
    {
        crash_execution("strcmp_impl: Argument 1 is not valid C string.");
        return;
    }
    auto const rhs{ parameters().at(2).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)rhs))
    {
        crash_execution("strcmp_impl: Argument 2 is not valid C string.");
        return;
    }
    auto const dst_ptr{ parameters().front().read<int*>() };
    *dst_ptr = std::strcmp(lhs, rhs);
}


void ExternCodeCStd::strncmp_impl()
{
    auto const count{ parameters().at(3).read<int>() };
    auto const lhs{ parameters().at(1).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)lhs, std::max(count, 0)))
    {
        crash_execution("strncmp_impl: Argument 1 is not valid C string.");
        return;
    }
    auto const rhs{ parameters().at(2).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)lhs, std::max(count, 0)))
    {
        crash_execution("strncmp_impl: Argument 2 is not valid C string.");
        return;
    }
    auto const dst_ptr{ parameters().front().read<int*>() };
    *dst_ptr = std::strncmp(lhs, rhs, count);
}


void ExternCodeCStd::getopt_impl()
{
    auto const argc{ parameters().at(1).read<int>() };
    if (argc < 0)
    {
        crash_execution("getopt_impl: Argument 1 (argc) is negative.");
        return;
    }
    auto const argv{ parameters().at(2).read<char**>() };
    if (!sanitizer()->is_memory_valid((MemPtr)argv, argc * sizeof(char*)))
    {
        crash_execution("getopt_impl: Argument 2 does not points to valid memory.");
        return;
    }
    for (int i = 0; i != argc; ++i)
        if (!sanitizer()->is_c_string_valid((MemPtr)argv[i]))
        {
            crash_execution("getopt_impl: Argument 2 is not an array of valid C strings.");
            return;
        }
    auto const opt_string{ parameters().at(3).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)opt_string))
    {
        crash_execution("getopt_impl: Argument 3 is not valid C string.");
        return;
    }
    auto const dst_ptr{ parameters().front().read<int*>() };
    *dst_ptr = getopt(argc, argv, opt_string);
}


void ExternCodeCStd::getopt_long_impl()
{
    auto const argc{ parameters().at(1).read<int>() };
    if (argc < 0)
    {
        crash_execution("getopt_long_impl: Argument 1 (argc) is negative.");
        return;
    }
    auto const argv{ parameters().at(2).read<char**>() };
    if (!sanitizer()->is_memory_valid((MemPtr)argv, argc * sizeof(char*)))
    {
        crash_execution("getopt_long_impl: Argument 2 does not points to valid memory.");
        return;
    }
    for (int i = 0; i != argc; ++i)
        if (!sanitizer()->is_c_string_valid((MemPtr)argv[i]))
        {
            crash_execution("getopt_long_impl: Argument 2 is not an array of valid C strings.");
            return;
        }
    auto const opt_string{ parameters().at(3).read<char const*>() };
    if (!sanitizer()->is_c_string_valid((MemPtr)opt_string))
    {
        crash_execution("getopt_long_impl: Argument 3 is not valid C string.");
        return;
    }
    auto const longopts{ parameters().at(4).read<struct option const*>() };
    for (int i = 0; true; ++i)
    {
        if (!sanitizer()->is_memory_valid((MemPtr)(longopts + i), sizeof(struct option)))
        {
            crash_execution("getopt_long_impl: Argument 4 does not points to valid memory.");
            return;
        }
        if (longopts[i].flag != nullptr && !sanitizer()->is_memory_valid((MemPtr)(longopts[i].flag), sizeof(int)))
        {
            crash_execution("getopt_long_impl: Argument 4 has invalid pointer in the 'flag' field.");
            return;
        }
        if (longopts[i].name == nullptr && longopts[i].has_arg == 0 && longopts[i].flag == nullptr && longopts[i].val == 0)
            break;
    }
    auto const longindex{ parameters().at(5).read<int*>() };
    if (longindex != nullptr && !sanitizer()->is_memory_valid((MemPtr)longindex, sizeof(int)))
    {
        crash_execution("getopt_long_impl: Argument 5 does not points to valid memory.");
        return;
    }
    auto const dst_ptr{ parameters().front().read<int*>() };
    *dst_ptr = getopt_long(argc, argv, opt_string, longopts, longindex);
}


}
