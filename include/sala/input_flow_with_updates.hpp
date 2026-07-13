#ifndef SALA_INPUT_FLOW_WITH_UPDATES_HPP_INCLUDED
#   define SALA_INPUT_FLOW_WITH_UPDATES_HPP_INCLUDED

#   include <sala/input_flow.hpp>
#   include <unordered_map>
#   include <memory>
#   include <cstdint>

namespace sala {


struct InputFlowWithUpdates : public InputFlow
{
    struct InstructionID
    {
        bool operator==(InstructionID const& other) const
        { return func == other.func && block == other.block && instr == other.instr; }

        std::uint32_t func;
        std::uint32_t block;
        std::uint32_t instr;
    };

    struct InstructionIDHasher
    {
        std::uint64_t operator()(InstructionID const& id) const
        { return 31ULL * (31ULL * id.func + id.block) + id.instr; }
    };

    using  UpdatesHitCounts = std::unordered_map<InstructionID, std::uint32_t, InstructionIDHasher>;
    using  UpdatesHitCountsPtr = std::shared_ptr<UpdatesHitCounts>;

    explicit InputFlowWithUpdates(ExecState* exec_state);

    void start(MemPtr ptr, InputDescriptor desc) override;
    void start(MemPtr ptr, std::size_t count, InputDescriptor desc) override;
    void copy(MemPtr dst, MemPtr src, std::size_t count) override;
    void slice(MemPtr dst, MemPtr src, std::size_t count) override;
    void set(MemPtr dst, MemPtr ptr, std::size_t count) override;
    void move(MemPtr dst, MemPtr ptr, std::size_t count) override;
    void clear(MemPtr dst, std::size_t count) override;
    void join(MemPtr dst, std::size_t count, std::vector<std::pair<MemPtr, std::size_t> > const& memory) override;
    void join_per_byte(MemPtr dst, MemPtr src1, MemPtr src2, std::size_t count) override;
    void join_extend(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count) override;
    void extend_signed(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count) override;
    void extend_unsigned(MemPtr dst, std::size_t dst_count, MemPtr src, std::size_t src_count) override;

    UpdatesHitCountsPtr  read_updates(MemPtr ptr) const;

private:

    void write_updates(MemPtr ptr, UpdatesHitCountsPtr updates);
    void merge_updates(MemPtr ptr, UpdatesHitCountsPtr updates);
    void merge_updates(UpdatesHitCounts& dest, UpdatesHitCounts const& updates);

    // We track hit-counts for memory locations directly. That could be
    // memory expensive. If that shows to be a problem, then we can introduce
    // and track handles to hit-counts (similar to InputFlow::FlowSetHandle).
    std::unordered_map<MemPtr, UpdatesHitCountsPtr> m_updates;
};


}

#endif
