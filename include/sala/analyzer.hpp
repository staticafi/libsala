#ifndef SALA_ANALYZER_HPP_INCLUDED
#   define SALA_ANALYZER_HPP_INCLUDED

#   include <sala/exec_state.hpp>
#   include <sala/instr_switch.hpp>
#   include <functional>


namespace sala {


struct Analyzer : public InstrSwitch
{
    using PostOperation = std::function<void()>;

    explicit Analyzer(ExecState* state);
    virtual ~Analyzer() {}

    ExecState const& state() const { return *state_; }
    ExecState& state() { return *state_; }

    Program const& program() const { return state().program(); }
    Instruction const& instruction() const override { return state().current_instruction(); }
    std::vector<MemBlock const*> const& operands() const override { return state().current_operands(); }
    StackRecord const& stack_top() const { return state().stack_top(); }
    InstrPointer const& ip() const { return stack_top().ip(); }

    void set_post_operation(PostOperation const& post_op) { post_operation_ = post_op; }

    virtual void on_stack_initialized() {}

    void pre() { post_operation_ = nullptr; do_instruction_switch(); }
    void post() { if (post_operation_) post_operation_(); }

private:
    ExecState* state_;
    PostOperation post_operation_;
};


}

#endif
