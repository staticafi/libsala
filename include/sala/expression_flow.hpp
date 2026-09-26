#ifndef SALA_EXPRESSION_FLOW_HPP_INCLUDED
#   define SALA_EXPRESSION_FLOW_HPP_INCLUDED

#   include <sala/analyzer.hpp>
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
    explicit Expression(InstructionHandle const& h)
        : m_handle{ h }
    {}
    virtual ~Expression() {}
    InstructionHandle handle() const { return m_handle; }
    virtual bool is_input_source() const { return false; }
    virtual bool is_constant_source() const { return false; }
    virtual std::uint32_t num_children() const { return 0U; }
    virtual ExpressionPtr child(std::uint32_t const idx) const { return nullptr; }
private:
    InstructionHandle m_handle;
};


struct ExpressionInput : public Expression
{
    explicit ExpressionInput(InstructionHandle const& h)
        : Expression{ h }
    {}

    bool is_input_source() const override { return true; }
};


struct ExpressionConstant : public Expression
{
    explicit ExpressionConstant(InstructionHandle const& h, std::uint32_t const idx)
        : Expression{ h }
    {}

    bool is_input_source() const override { return true; }
};


struct ExpressionFlow : public Analyzer
{

    explicit ExpressionFlow(ExecState* exec_state);

private:

};


}

#endif
