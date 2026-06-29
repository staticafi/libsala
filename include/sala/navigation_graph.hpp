#ifndef SALA_NAVIGATION_GRAPH_HPP_INCLUDED
#   define SALA_NAVIGATION_GRAPH_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/call_graph.hpp>
#   include <sala/control_flow_graph.hpp>
#   include <utility/std_pair_hash.hpp>
#   include <unordered_set>
#   include <unordered_map>
#   include <limits>
#   include <cstdint>
#   include <vector>

namespace sala {


struct  NavigationGraph : public ControlFlowGraph
{
    // Term 'cost' in this graph means 'the count of instructions'.
    // A cost is always a positive integer.
    using Cost = std::uint32_t;

    static constexpr Cost  INFINITY_COST = std::numeric_limits<Cost>::max();

    static inline Cost sum_costs(Cost x, Cost y)
    { return  (Cost)std::min((std::uint64_t)INFINITY_COST, (std::uint64_t)x + (std::uint64_t)y); }

    // For each function we have a table t of type CostTable
    // of costs for transition between nodes. So, if u,v are indices
    // of nodes in the same function, then T[{u,v}] is the minimal cost
    // of moving from u to v in the function. If {u,v} is not in the
    // table, then the cost is assumed to be INFINITY_COST. If the
    // shortest path goes through CALL node, then value in m_calls_avg_costs
    // was used to compute the cost of the path.
    using CostTable = std::unordered_map<std::pair<std::uint32_t, std::uint32_t>, Cost>;


    NavigationGraph(Program const&  program, CallGraph const&  cg);

    // Average costs of functions:

    std::vector<Cost> const&  func_avg_costs() const { return m_func_avg_costs; }
    Cost  func_avg_cost(std::uint32_t const  func_index) const { return m_func_avg_costs.at(func_index); }

    // Average costs of calls inside functions:

    std::vector<Cost> const&  calls_avg_costs(std::uint32_t const  func_index) const { return m_calls_avg_costs.at(func_index); }
    Cost  call_avg_cost(std::uint32_t const  call_node_index) const;

    // Costs of for transitions between nodes within one function:

    // For each function we have one table.
    std::vector<CostTable> const&  intra_costs() const { return m_intra_costs; }
    // Returns cost of moving from node from_node_index to to_node_index.
    // Both nodes are assumed to be in the same function. Otherwise the
    // the result is undefined.
    Cost  intra_cost(std::uint32_t  from_node_index, std::uint32_t  to_node_index) const;

private:

    void  compute_avg_costs(CallGraph const&  cg);
    void  update_avg_costs_rec(
            std::uint32_t  func_index,
            std::unordered_set<std::uint32_t>&  resolved_functions,
            CallGraph const&  cg
            );

    void  compute_intra_costs();
    void compute_intra_costs(std::uint32_t  func_index);

    // For each function we have a cost.
    // Initial cost of a function is the total count of instructions
    // in the functions (sum of instruction counts in all its basic
    // blocks). Final cost is a sum of the initial cost and final
    // costs of functions called in CALL nodes (when one CALL node
    // calls several functions, then average of their final costs
    // is considered).
    std::vector<Cost>  m_func_avg_costs;

    // For each function we have a vector of average costs.
    // Each cost correspond to a node in FunctionLookup::calls.
    // The cost is the average of costs in m_func_avg_costs
    // restricted to the successors of the node.
    std::vector<std::vector<Cost> >  m_calls_avg_costs;

    // For each function we keep one table of costs (for moves from
    // one node to another within the same function).
    std::vector<CostTable>  m_intra_costs;
};


}

#endif
