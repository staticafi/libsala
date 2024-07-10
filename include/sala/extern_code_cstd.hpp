#ifndef SALA_EXTERN_CODE_CSTD_HPP_INCLUDED
#   define SALA_EXTERN_CODE_CSTD_HPP_INCLUDED

#   include <sala/extern_code.hpp>

namespace sala {


struct ExternCodeCStd : public ExternCode
{
    explicit ExternCodeCStd(ExecState* const state);

private:
    void register_math_functions();
    void register_fenv_functions();
};


}

#endif
