#include <sala/control_flow_graph.hpp>
#include <utility/invariants.hpp>
#include <utility/assumptions.hpp>
#include <unordered_map>
#include <algorithm>

namespace sala {


ControlFlowGraph::ControlFlowGraph(Program const&  program, CallGraph const&  cg)
    : m_nodes{}
    , m_lookups{}
{
    for (Function const&  func : program.functions())
    {
        m_lookups.push_back(FunctionLookup{
            .begin = (std::uint32_t)m_nodes.size(),
            .end = (std::uint32_t)m_nodes.size(),
            .bbs{},
            .calls{},
            .rets{}
        });
        for (BasicBlock const&  bb : func.basic_blocks())
        {
            m_lookups.back().bbs.push_back((std::uint32_t)m_nodes.size());
            for (Instruction const&  instr : bb.instructions())
            {
                switch (instr.opcode())
                {
                    case Instruction::Opcode::CALL:
                        m_lookups.back().calls.push_back((std::uint32_t)m_nodes.size());
                        break;
                    case Instruction::Opcode::RET:
                        m_lookups.back().rets.push_back((std::uint32_t)m_nodes.size());
                        break;
                    case Instruction::Opcode::JUMP:
                    case Instruction::Opcode::BRANCH:
                        break;
                    default:
                        continue;
                }
                m_nodes.push_back(Node{
                    .function = func.index(),
                    .basic_block = bb.index(),
                    .instruction = instr.index(),
                    .successors{}
                });
                ++m_lookups.back().end;
            }
            INVARIANT(m_lookups.back().end > m_lookups.back().begin);
        }
        m_lookups.back().bbs.push_back((std::uint32_t)m_nodes.size());
        INVARIANT(m_lookups.back().bbs.back() == m_lookups.back().end);
        INVARIANT(m_lookups.back().bbs.size() == func.basic_blocks().size() + 1ULL);
    }

    for (std::size_t  node_index = 0ULL; node_index != m_nodes.size(); ++node_index)
    {
        Node&  n = m_nodes.at(node_index);

        if (is_call(node_index))
        {
            for (std::uint32_t  func : cg.at(n.function)
                                         .at(n.basic_block)
                                         .at(n.instruction))
                n.successors.push_back(entry(func));
            INVARIANT(!n.successors.empty());
        }
        else
            for (std::uint32_t  bb : program.functions().at(n.function)
                                            .basic_blocks().at(n.basic_block)
                                            .successors())
                n.successors.push_back(bb_entry(n.function, bb));

        std::sort(n.successors.begin(), n.successors.end());
    }
}


bool  ControlFlowGraph::is_successor(std::uint32_t const  node_index, std::uint32_t const  checked_node_index) const
{
    auto const&  succ = node(node_index).successors;
    return  std::binary_search(succ.begin(), succ.end(), checked_node_index);
}


bool  ControlFlowGraph::is_call(std::uint32_t const  node_index) const
{
    auto const&  calls = lookup(node(node_index).function).calls;
    return  std::binary_search(calls.begin(), calls.end(), node_index);
}


bool  ControlFlowGraph::is_ret(std::uint32_t const  node_index) const
{
    auto const&  rets = lookup(node(node_index).function).rets;
    return  std::binary_search(rets.begin(), rets.end(), node_index);
}


std::uint32_t  ControlFlowGraph::num_instructions(std::uint32_t const  node_index) const
{
    Node const&  n = node(node_index);
    std::uint32_t pred_node_index = bb_entry(n.function, n.basic_block);
    if (pred_node_index == node_index)
        return  n.instruction + 1U;
    while (pred_node_index + 1U != node_index)
        ++pred_node_index;
    return  n.instruction - node(pred_node_index).instruction;
}


std::uint32_t  ControlFlowGraph::bb_next(std::uint32_t const  node_index) const
{
    ASSUMPTION(is_call(node_index));
    return  node_index + 1U;
}


std::uint32_t  ControlFlowGraph::bb_of_instruction(
        std::uint32_t const  function_index,
        std::uint32_t const  bb_index,
        std::uint32_t const  instr_index
        ) const
{
    for (auto [i, e] = bb_range(function_index, bb_index); i != e; ++i)
        if (instr_index <= node(i).instruction)
            return i;
    UNREACHABLE();
}


}
