#include <sala/extern_code_cstd.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <cmath>
#include <cfenv>

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

namespace sala {


ExternCodeCStd::ExternCodeCStd(ExecState* const state)
    : ExternCode{ state }
{
    register_math_functions();
    register_fenv_functions();
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


void ExternCodeCStd::register_fenv_functions()
{
    REGISTER_CONST_FUNC(fegetround, int);
    REGISTER_UNARY_FUNC(fesetround, int);
}


}
