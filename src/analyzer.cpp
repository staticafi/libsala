#include <sala/analyzer.hpp>

namespace sala {


Analyzer::Analyzer(ExecState* const state)
    : state_{ state }
    , post_operation_{}
    , extern_function_processors_{}
{}


void Analyzer::register_extern_function_processor(std::string const& function_name, std::function<void()> const& code)
{
    extern_function_processors_.insert_or_assign(function_name, code);
}


void Analyzer::call_processor_of_current_function_if_registered_extern()
{
    if (state().current_function().is_external())
    {
        auto const it{ extern_function_processors_.find(state().current_function().name()) };
        if (it != extern_function_processors_.end())
            it->second();
    }
}


}
