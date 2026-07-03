#include <sala/navigation_graph.hpp>
#include <utility/invariants.hpp>
#include <utility/assumptions.hpp>
#include <unordered_map>
#include <algorithm>

namespace sala {


NavigationGraph::NavigationGraph(Program const&  program, CallGraph const&  cg)
    : ControlFlowGraph{ program, cg }
    , m_func_avg_costs{}
    , m_calls_avg_costs{}
    , m_intra_costs{}
    , m_inter_costs{}
{
    compute_avg_costs(cg);
    compute_intra_costs();
    compute_inter_costs(cg);
}


void  NavigationGraph::compute_avg_costs(CallGraph const&  cg)
{
    // Initialization of everything to zero.

    m_func_avg_costs.resize(lookups().size(), 0U);
    m_calls_avg_costs.reserve(lookups().size());
    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
        m_calls_avg_costs.emplace_back(std::vector<Cost>(lookup(func_index).calls.size(), 0U));

    // Computation of initial costs to m_func_avg_costs, i.e., counts of all instructions in each function.

    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
    {
        Cost& cost = m_func_avg_costs.at(func_index);
        FunctionLookup const& l = lookup(func_index);
        for (std::uint32_t  node_index = l.begin; node_index != l.end; ++node_index)
            cost += num_instructions(node_index);
    }

    // Update of costs (both m_func_avg_costs and m_calls_avg_costs) recursively by propagating
    // and accumulating costs of callees to callers.

    std::unordered_set<std::uint32_t>  resolved_functions{};
    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
        update_avg_costs_rec(func_index, resolved_functions, cg);
}


void  NavigationGraph::update_avg_costs_rec(
        std::uint32_t  func_index,
        std::unordered_set<std::uint32_t>&  resolved_functions,
        CallGraph const&  cg
        )
{
    if (resolved_functions.contains(func_index))
        return;
    resolved_functions.insert(func_index);

    Cost&  func_cost = m_func_avg_costs.at(func_index);
    std::vector<Cost>&  calls_costs = m_calls_avg_costs.at(func_index);
    FunctionLookup const& l = lookup(func_index);
    for (std::uint32_t  call_index = 0U; (std::size_t)call_index != l.calls.size(); ++call_index)
    {
        Node const&  call = node(l.calls.at(call_index));
        Cost  sum{ 0U };
        for (std::uint32_t const  succ_node_index : call.successors)
        {
            Node const&  succ = node(succ_node_index);
            update_avg_costs_rec(succ.function, resolved_functions, cg);
            sum += m_func_avg_costs.at(succ.function);
        }
        Cost cost = std::max(1U, (std::uint32_t)(sum / call.successors.size()));
        calls_costs.at(call_index) = cost;
        func_cost += cost;
    }
}


void  NavigationGraph::compute_intra_costs()
{
    m_intra_costs.reserve(lookups().size());
    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
        compute_intra_costs(func_index);
}


void  NavigationGraph::compute_intra_costs(std::uint32_t const  func_index)
{
    FunctionLookup const& l = lookup(func_index);

    m_intra_costs.push_back({});
    IntraCostTable&  table = m_intra_costs.back();

    // Initialization of costs table for the Floyd-Warshall algorithm.
    // Zeros are at diagonal, instruction counts for successors
    // and INFINITY_COST everywhere else.
    // NOTE: We do NOT keep mappings to INFINITY_COST cost.

    for (std::uint32_t  u = l.begin; u != l.end; ++u)
    {
        table.insert({ { u, u } , 0U });
        if (is_call(u))
        {
            std::uint32_t const  v = bb_next(u);
            Cost  cost = sum_costs((Cost)num_instructions(u), call_avg_cost(u));
            if (is_ret(v))
                cost = sum_costs((Cost)num_instructions(v), cost);
            table.insert({ { u, v }, cost });
        }
        else
        {
            Cost const  cost_u = (Cost)num_instructions(u);
            for (std::uint32_t const  v : successors(u))
                table[{u, v}] = cost_u + (is_ret(v) ? (Cost)num_instructions(v) : 0U);
        }
    }

    // Actual computation of costs using the Floyd-Warshall algorithm.

    for (std::uint32_t  x = l.begin; x != l.end; ++x)
    {
        IntraCostTable  new_table;
        for (std::uint32_t  u = l.begin; u != l.end; ++u)
            for (std::uint32_t  v = l.begin; v != l.end; ++v)
            {
                Cost const  cost = intra_cost(u, v);
                Cost const  other_cost = sum_costs(intra_cost(u, x), intra_cost(x, v));
                if (other_cost < cost)
                    new_table[{u, v}] = other_cost;
                if (cost != INFINITY_COST)
                    new_table[{u, v}] = cost;

            }
        table.swap(new_table);
    }

    // We must further update costs for diagonal elements.
    // They are currently zero. They should represent
    // costs of shortest path from that node back to
    // it (i.e., leaving the node and returning back).

    for (std::uint32_t  u = l.begin; u != l.end; ++u)
    {
        std::vector<std::uint32_t>  succ;
        if (is_call(u))
            succ = { bb_next(u) };
        else
            succ = successors(u);

        if (std::find(succ.begin(), succ.end(), u) != succ.end())
            table[{u, u}] = num_instructions(u);
        else
        {
            table.erase({ u, u });
            Cost cost = INFINITY_COST;
            for (std::uint32_t const  v : succ)
            {
                Cost const  other_cost = sum_costs(intra_cost(u, v), intra_cost(v, u));
                if (other_cost < cost)
                {
                    table[{u, u}] = other_cost;
                    cost = other_cost;
                }
            }
        }
    }
}


void  NavigationGraph::compute_inter_costs(CallGraph const&  cg)
{
    m_inter_costs.resize(lookups().size());
    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
        m_inter_costs.at(func_index).from_calls.resize(calls(func_index).size());
    
    std::unordered_set<std::uint32_t>  resolved_functions{};
    for (std::uint32_t  func_index = 0U; (std::size_t)func_index != lookups().size(); ++func_index)
        update_inter_costs_rec(func_index, resolved_functions, cg);
}


void  NavigationGraph::update_inter_costs_rec(
        std::uint32_t  func_index,
        std::unordered_set<std::uint32_t>&  resolved_functions,
        CallGraph const&  cg
        )
{
    if (resolved_functions.contains(func_index))
        return;
    resolved_functions.insert(func_index);

    InterCosts&  costs = m_inter_costs.at(func_index);
    FunctionLookup const& l = lookup(func_index);
    for (std::uint32_t  call_index = 0U; (std::size_t)call_index != l.calls.size(); ++call_index)
    {
        std::uint32_t const  call_node_index = l.calls.at(call_index);
        Node const&  call = node(call_node_index);
        InterCosts::Table&  call_costs = costs.from_calls.at(call_index);
        Cost const  instr_count_to_call = num_instructions(call_node_index);

        for (std::uint32_t const  succ_node_index : call.successors)
            call_costs[succ_node_index] = instr_count_to_call;

        for (std::uint32_t const  succ_node_index : call.successors)
        {
            Node const&  succ = node(succ_node_index);
            update_inter_costs_rec(succ.function, resolved_functions, cg);

            InterCosts::Table&  callee_costs = m_inter_costs.at(succ.function).from_entry;
            for (auto [ func, cost ] : callee_costs)
            {
                auto const  it = call_costs.find(func);
                call_costs[func] = (it == call_costs.end()) ? cost : std::min(cost, it->second);
            }
        }

        Cost const  local_cost = is_entry(call_node_index) ?
            instr_count_to_call :
            intra_cost(entry(func_index), call_node_index);
        for (auto [ func, cost ] : call_costs)
        {
            Cost const  new_cost = sum_costs(local_cost, cost);
            auto const  it = costs.from_entry.find(func);
            costs.from_entry[func] = (it == costs.from_entry.end()) ? new_cost : std::min(new_cost, it->second);
        }
    }
}


NavigationGraph::Cost  NavigationGraph::call_avg_cost(std::uint32_t const  call_node_index) const
{
    std::uint32_t const  func = node(call_node_index).function;
    auto const&  calls = lookup(func).calls;
    auto const  it = std::lower_bound(calls.begin(), calls.end(), call_node_index);
    ASSUMPTION(it != calls.end() && *it == call_node_index);
    std::size_t const  index = it - calls.begin();
    return  m_calls_avg_costs.at(func).at(index);
}


NavigationGraph::Cost  NavigationGraph::intra_cost(std::uint32_t const  from_node_index, std::uint32_t const  to_node_index) const
{
    ASSUMPTION(node(from_node_index).function == node(to_node_index).function);
    auto const&  table = m_intra_costs.at(node(from_node_index).function);
    auto const  it = table.find({ from_node_index, to_node_index });
    return  it == table.end() ? INFINITY_COST : it->second;
}


NavigationGraph::Cost  NavigationGraph::inter_cost_from_entry(std::uint32_t const  from_function_index, std::uint32_t const  to_function_index) const
{
    auto const&  costs = m_inter_costs.at(from_function_index).from_entry;
    auto const  it = costs.find(to_function_index);
    return  it == costs.end() ? INFINITY_COST : it->second;
}


NavigationGraph::Cost  NavigationGraph::inter_cost_from_call(std::uint32_t const  call_node_index, std::uint32_t const  to_function_index) const
{
    std::uint32_t const  func = node(call_node_index).function;
    auto const&  calls = lookup(func).calls;
    auto const  cit = std::lower_bound(calls.begin(), calls.end(), call_node_index);
    ASSUMPTION(cit != calls.end() && *cit == call_node_index);
    std::size_t const  index = cit - calls.begin();
    auto const&  costs = m_inter_costs.at(node(call_node_index).function).from_calls.at(index);
    auto const  it = costs.find(to_function_index);
    return  it == costs.end() ? INFINITY_COST : it->second;
}


}
