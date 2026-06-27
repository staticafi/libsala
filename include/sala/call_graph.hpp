#ifndef SALA_CALL_GRAPH_HPP_INCLUDED
#   define SALA_CALL_GRAPH_HPP_INCLUDED

#   include <sala/program.hpp>
#   include <cstdint>
#   include <unordered_map>
#   include <vector>

namespace sala {


using CallGraph =
    std::unordered_map<std::uint32_t, // from function
        std::unordered_map<std::uint32_t,  // from basic block
            std::unordered_map<std::uint32_t,  // from instruction
            std::vector<std::uint32_t>  // to functions (may be several, when calling via pointer)
            > > >;


CallGraph  make_call_graph(Program const&  program);


}

#endif
