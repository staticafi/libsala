#ifndef SALA_EXEC_STATE_HPP_INCLUDED
#   define SALA_EXEC_STATE_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/memblock.hpp>
#   include <sala/pointer_model.hpp>
#   include <vector>
#   include <unordered_map>
#   include <memory>
#   include <cstdint>

namespace sala {


struct InstrPointer final
{
    InstrPointer();

    std::uint32_t block() const { return block_; }
    std::uint32_t instr() const { return instr_; }

    void next();
    void jump(std::uint32_t const new_block_idx);

private:
    std::uint32_t block_;
    std::uint32_t instr_;
};


struct StackRecord final
{
    StackRecord();
    explicit StackRecord(PointerModel* pointer_model, Function const& F);

    std::uint32_t function_index() const { return function_index_; }
    InstrPointer const& ip() const { return ip_; }
    InstrPointer& ip() { return ip_; }

    std::vector<MemBlock> const& parameters() const { return parameters_; }
    std::vector<MemBlock> const& locals() const { return locals_; }
    std::vector<MemBlock> const& variadic_parameters() const { return variadic_parameters_; }

    void push_back_variadic_parameter(std::size_t num_bytes);

    void push_back_local_variable(std::size_t num_bytes);
    void pop_back_local_variable();

private:
    PointerModel* pointer_model_;
    std::uint32_t function_index_;
    InstrPointer ip_;
    std::vector<MemBlock> parameters_;
    std::vector<MemBlock> locals_;
    std::vector<MemBlock> variadic_parameters_;
};


struct ExecState final
{
    enum struct Stage
    {
        INITIALIZING    = 0,
        EXECUTING       = 1,
        TERMINATING     = 2,
        FINISHED        = 3
    };

    enum struct Termination
    {
        UNKNOWN = -1,
        NORMAL  = 0,
        ERROR   = 1,
        CRASH   = 2
    };

    explicit ExecState(Program const* P);
    ~ExecState();

    Program const& program() const { return *program_; }
    PointerModel* pointer_model() { return pointer_model_; }

    Stage stage() const { return stage_; }
    Termination termination() const { return termination_; }
    std::string const& terminator() const { return terminator_; }
    std::string const& error_message() const { return error_message_; }
    Instruction const* termination_instruction() const { return termination_instruction_; }
    int exit_code() const { return exit_code_.read<int>(); }
    MemBlock const& exit_code_memory_block() const { return exit_code_; }

    std::vector<MemBlock> const& constant_segment() const { return constant_segment_; }
    std::vector<MemBlock> const& static_segment() const { return static_segment_; }
    std::vector<MemBlock> const& function_segment() const { return function_segment_; }
    std::unordered_map<MemPtr, std::uint32_t> const& functions_at_addresses() const { return functions_at_addresses_; }
    std::vector<StackRecord> const& stack_segment() const { return stack_segment_; }
    StackRecord const& stack_top() const { return stack_segment_.back(); }
    InstrPointer const& ip() const { return stack_top().ip(); }
    std::unordered_map<MemPtr, MemBlock> const& heap_segment() const { return heap_segment_; }

    Function const& current_function() const { return *current_function_; }
    BasicBlock const& current_block() const { return *current_block_; }
    Instruction const& current_instruction() const { return *current_instruction_; }
    std::vector<MemBlock const*> const& current_operands() const { return current_operands_; }

    std::vector<StackRecord>& stack_segment() { return stack_segment_; }
    StackRecord& stack_top() { return stack_segment_.back(); }
    std::size_t stack_exit_depth() const { return stack_exit_depth_; }
    std::unordered_map<MemPtr, MemBlock>& heap_segment() { return heap_segment_; }

    std::vector<std::uint32_t> const& atexit_stack() const { return atexit_stack_; }
    void push_atexit_function(std::uint32_t const func_index) { atexit_stack_.push_back(func_index); }
    std::uint32_t pop_atexit_function() { auto const func_index{ atexit_stack_.back() }; atexit_stack_.pop_back(); return func_index; }

    void set_stack_exit_depth(std::size_t const size) { stack_exit_depth_ = size; }

    bool set_stage(Stage type);
    bool set_termination(Termination type, std::string const& terminator, std::string const& message, Instruction const* instruction = nullptr);
    void set_exit_code(std::int32_t const c) { exit_code_.write(c); }

    void update_current_values();

    std::string current_location_message() const;
    std::string make_error_message(std::string const& text) const;

private:

    Program const* program_;
    PointerModel* pointer_model_;

    Stage stage_;
    Termination termination_;
    std::string terminator_;
    std::string error_message_;
    Instruction const* termination_instruction_;
    MemBlock exit_code_;

    std::vector<MemBlock> constant_segment_;
    std::vector<MemBlock> static_segment_;
    std::vector<MemBlock> function_segment_;
    std::unordered_map<MemPtr, std::uint32_t> functions_at_addresses_;
    std::vector<StackRecord> stack_segment_;
    std::unordered_map<MemPtr, MemBlock> heap_segment_;

    std::size_t stack_exit_depth_;

    std::vector<std::uint32_t> atexit_stack_;

    Function const* current_function_;
    BasicBlock const* current_block_;
    Instruction const* current_instruction_;
    std::vector<MemBlock const*> current_operands_;
}; 


}

#endif
