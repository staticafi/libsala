#ifndef SALA_CONTROL_FLOW_GRAPH_HPP_INCLUDED
#   define SALA_CONTROL_FLOW_GRAPH_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <sala/call_graph.hpp>
#   include <cstdint>
#   include <vector>

namespace sala {


struct  CFNode
{
    std::uint32_t  function;
    std::uint32_t  basic_block;
    std::uint32_t  instruction; // the instruction in which this node ends.
    bool  ends_in_call;
    std::vector<std::uint32_t>  successors;
};


using  CFGraph = std::vector<CFNode>;


CFGraph  make_control_flow_graph(Program const&  program, CallGraph const&  cg);


}

#endif
