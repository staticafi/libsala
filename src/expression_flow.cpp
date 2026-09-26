#include <sala/expression_flow.hpp>
#include <sala/platform_specifics.hpp>
#include <utility/hash_combine.hpp>
#include <utility/assumptions.hpp>
#include <utility/invariants.hpp>
#include <utility/development.hpp>

namespace sala {


Expression::Expression(
        InstructionHandle const& handle_,
        Kind const kind_,
        ExpressionPtr const guard_,
        std::vector<ExpressionPtr> const& children_
        )
    : m_handle{ handle_ }
    , m_kind{ kind_ }
    , m_guard{ guard_ }
    , m_children{ children_ }
{}


}
