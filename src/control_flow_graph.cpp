#include <sala/control_flow_graph.hpp>
#include <utility/std_pair_hash.hpp>
#include <unordered_map>
#include <numeric>

namespace sala {


CFGraph  make_control_flow_graph(Program const&  program, CallGraph const&  cg)
{
    CFGraph  cfg;

    std::unordered_map<std::uint32_t, std::size_t>  func_entries;
    std::unordered_map<std::pair<std::uint32_t, std::uint32_t>, std::size_t>  bb_entries;

    std::vector<std::uint32_t> func_indices(program.functions().size());
    std::iota(func_indices.begin(), func_indices.end(), 1);
    std::swap(func_indices[0], func_indices[program.entry_function() - 1U]);

    for (std::uint32_t  func_index : func_indices)
    {
        bool is_func_entry = true;
        for (BasicBlock const& bb : program.functions().at(func_index).basic_blocks())
        {
            bool is_bb_entry = true;
            for (Instruction const& instr : bb.instructions())
            {
                bool  ends_in_call;
                switch (instr.opcode())
                {
                    case Instruction::Opcode::CALL:
                        ends_in_call = true;
                        break;
                    case Instruction::Opcode::RET:
                    case Instruction::Opcode::JUMP:
                    case Instruction::Opcode::BRANCH:
                        ends_in_call = false;
                        break;
                    default:
                        continue;
                }
                if (is_func_entry)
                {
                    func_entries.insert({ func_index, cfg.size() });
                    is_func_entry = false;
                }
                if (is_bb_entry)
                {
                    bb_entries.insert({ { func_index, bb.index() }, cfg.size() });
                    is_bb_entry = false;
                }
                cfg.push_back(CFNode{
                    .function = func_index,
                    .basic_block = bb.index(),
                    .instruction = instr.index(),
                    .ends_in_call = ends_in_call,
                    .successors{}
                });
            }
        }
    }

    for (CFNode& n : cfg)
        if (n.ends_in_call)
            for (std::uint32_t  func : cg.at(n.function)
                                         .at(n.basic_block)
                                         .at(n.instruction))
                n.successors.push_back(func_entries.at(func));
        else
            for (std::uint32_t  bb : program.functions().at(n.function)
                                            .basic_blocks().at(n.basic_block)
                                            .successors())
                n.successors.push_back(bb_entries.at({ n.function, bb }));

    return cfg;
}


}
