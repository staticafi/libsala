#ifndef SALA_EXTERN_CODE_CSTD_HPP_INCLUDED
#   define SALA_EXTERN_CODE_CSTD_HPP_INCLUDED

#   include <sala/extern_code.hpp>

namespace sala {


struct ExternCodeCStd : public ExternCode
{
    explicit ExternCodeCStd(ExecState* const state, Sanitizer* const sanitizer);

private:
    void register_math_functions();
    void register_string_functions();
    void register_fenv_functions();
    void register_linux_functions();

    void crash_execution(std::string const& message);

    void strlen_impl();
    void strchr_impl();
    void strrchr_impl();
    void strspn_impl();
    void strcspn_impl();
    void strpbrk_impl();
    void strstr_impl();
    void strtok_impl();
    void strcat_impl();
    void strncat_impl();
    void strcpy_impl();
    void strncpy_impl();
    void strcmp_impl();
    void strncmp_impl();
    void getopt_impl();
    void getopt_long_impl();
};


}

#endif
