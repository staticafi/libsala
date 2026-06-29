#ifndef SALA_CONTROL_FLOW_GRAPH_HPP_INCLUDED
#   define SALA_CONTROL_FLOW_GRAPH_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/call_graph.hpp>
#   include <cstdint>
#   include <vector>

namespace sala {


struct ControlFlowGraph
{
    struct  Node
    {
        std::uint32_t  function;
        std::uint32_t  basic_block;

        // The instruction in which this node ends,
        // that is either CALL, RET, JUMP or BRANCH.
        std::uint32_t  instruction;
    
        // Nodes ending by CALL, RET, JUMP, BRANCH
        // instruction have any number, 0, 1, 2 indices
        // this vector, respectively.
        // Successors of nodes ending by CALL are entry
        // blocks of called functions (there can be more
        // than one callee in case of call via pointer.) 
        // The vector is sorted - ascending order.
        std::vector<std::uint32_t>  successors;
    };

    struct FunctionLookup
    {
        // Range of nodes in `m_nodes` which belong to the function.
        std::uint32_t  begin;   // This is also entry block to the function.
        std::uint32_t  end;     // The node at this index does NOT belong to the function.

        // For each basic block (BB) in the function there is
        // an index of the corresponding node in this vector.
        // Since a BB with CALL(s) inside is represented by
        // several nodes, all these these nodes can be found
        // using this vector. Namely, given BB index `i`, we
        // have range [ bbs[i], bbs[i+1] ) of node indices
        // representing that basic block. The last item in bbs
        // is the value `end` above, so that we can always form
        // the range of node indices, as shown above.
        std::vector<std::uint32_t>  bbs;

        // Nodes ending by CALL instruction. The vector is sorted - ascending order.
        std::vector<std::uint32_t>  calls;

        // Nodes ending by RET instruction. The vector is sorted - ascending order.
        std::vector<std::uint32_t>  rets;
    };

    ControlFlowGraph(Program const&  program, CallGraph const&  cg);

    // Working with nodes (their indices):

    std::vector<Node> const&  nodes() const { return m_nodes; }
    Node const&  node(std::uint32_t const  node_index) const { return m_nodes.at(node_index); }
    std::vector<std::uint32_t> const&  successors(std::uint32_t  node_index) const { return node(node_index).successors; }
    bool  is_entry(std::uint32_t const  node_index) const { return entry(node(node_index).function) == node_index; }
    bool  is_call(std::uint32_t  node_index) const;
    bool  is_ret(std::uint32_t  node_index) const;
    bool  is_successor(std::uint32_t  node_index, std::uint32_t  checked_node_index) const;
    std::uint32_t  num_instructions(std::uint32_t  node_index) const;

    // Working with function lookups (their/function indices):

    std::vector<FunctionLookup> const&  lookups() const { return m_lookups; }
    FunctionLookup const&  lookup(std::uint32_t const  function_index) const { return m_lookups.at(function_index); }
    std::uint32_t  begin(std::uint32_t const  function_index) const { return lookup(function_index).begin; }
    std::uint32_t  end(std::uint32_t const  function_index) const { return lookup(function_index).end; }
    std::uint32_t  entry(std::uint32_t const  function_index) const { return begin(function_index); }
    std::uint32_t  bb_entry(std::uint32_t const  function_index, std::uint32_t const  bb_index) const
    { return bbs(function_index).at(bb_index); }
    std::pair<std::uint32_t, std::uint32_t>  bb_range(std::uint32_t const  function_index, std::uint32_t const  bb_index) const
    { auto const&  vec = bbs(function_index); return { vec.at(bb_index), vec.at(bb_index + 1U) }; }
    std::vector<std::uint32_t> const&  bbs(std::uint32_t const  function_index) const { return lookup(function_index).bbs; }
    std::vector<std::uint32_t> const&  calls(std::uint32_t const  function_index) const { return lookup(function_index).calls; }
    std::vector<std::uint32_t> const&  rets(std::uint32_t const  function_index) const { return lookup(function_index).rets; }

private:

    std::vector<Node>  m_nodes;
    std::vector<FunctionLookup>  m_lookups;
};


}

#endif
