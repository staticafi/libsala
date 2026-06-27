#include <sala/call_graph.hpp>
#include <utility/invariants.hpp>

namespace sala {


CallGraph  make_call_graph(Program const&  program)
{
    CallGraph cg;
    for (Function const& func : program.functions())
        for (BasicBlock const& bb : func.basic_blocks())
            for (Instruction const& instr : bb.instructions())
                if (instr.opcode() == Instruction::Opcode::CALL)
                {
                    std::vector<std::uint32_t>&  targets = cg[func.index()][bb.index()][instr.index()];
                    if (instr.descriptors().front() == Instruction::Descriptor::FUNCTION)
                        targets.push_back(instr.operands().front());
                    else
                    {
                        for (Function const& other_func : program.functions())
                        {
                            if (other_func.parameters().size() > instr.operands().size() - 1ULL)
                                continue;
                            bool ok = true;
                            for (std::size_t  i = 0ULL; i != other_func.parameters().size(); ++i)
                            {
                                std::uint32_t const  var_idx = instr.operands().at(i + 1ULL);
                                std::size_t  num_bytes;
                                switch (instr.descriptors().at(i + 1ULL))
                                {
                                    
                                    case Instruction::Descriptor::STATIC:
                                        num_bytes = program.static_variables().at(var_idx).num_bytes();
                                        break;
                                    case Instruction::Descriptor::LOCAL:
                                        num_bytes = func.local_variables().at(var_idx).num_bytes();
                                        break;
                                    case Instruction::Descriptor::PARAMETER:
                                        num_bytes = func.parameters().at(var_idx).num_bytes();
                                        break;
                                    case Instruction::Descriptor::CONSTANT:
                                        num_bytes = program.constants().at(var_idx).num_bytes();
                                        break;
                                    default:
                                        UNREACHABLE();
                                        break;
                                }
                                if (num_bytes != other_func.parameters().at(i).num_bytes())
                                {
                                    ok = false;
                                    break;
                                }
                            }
                            if (ok)
                                targets.push_back(other_func.index());
                        }
                    }
                }
    return cg;
}


}
