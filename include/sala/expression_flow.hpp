#ifndef SALA_EXPRESSION_FLOW_HPP_INCLUDED
#   define SALA_EXPRESSION_FLOW_HPP_INCLUDED

#   include <sala/analyzer.hpp>
#   include <vector>
#   include <memory>
#   include <cstdint>

namespace sala {


struct Expression;
using ExpressionPtr = std::shared_ptr<Expression>;


struct InstructionHandle
{
    std::uint32_t graph_node_index;
    std::uint32_t instruction_index;
};


struct Expression
{
    enum Kind
    {
        INPUT,
        CONSTANT,
        OPERATOR
    };

    Expression(
        InstructionHandle const& handle_,
        Kind kind_,
        ExpressionPtr guard_,
        std::vector<ExpressionPtr> const& children_ = {}
        );
 
    InstructionHandle handle() const { return m_handle; }
    Kind kind() const { return m_kind; }
    ExpressionPtr guard() const { return m_guard; }

    bool is_input() const { return kind() == INPUT; }
    bool is_constant() const { return kind() == CONSTANT; }
    bool is_operator() const { return kind() == OPERATOR; }

    std::uint32_t num_children() const { return (std::uint32_t)m_children.size(); }
    ExpressionPtr child(std::uint32_t const idx) const { return m_children.at(idx); }
    std::vector<ExpressionPtr> const& children() const { return m_children; }

    void push_back_child(ExpressionPtr const child_) { m_children.push_back(child_); }

private:

    InstructionHandle m_handle;
    Kind m_kind;
    ExpressionPtr m_guard;
    std::vector<ExpressionPtr> m_children;
};


struct ExpressionFlow : public Analyzer
{

    explicit ExpressionFlow(ExecState* exec_state);

private:

};


}

#endif
