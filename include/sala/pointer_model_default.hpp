#ifndef SALA_POINTER_MODEL_DEFAULT_HPP_INCLUDED
#   define SALA_POINTER_MODEL_DEFAULT_HPP_INCLUDED

#   include <sala/pointer_model.hpp>

namespace sala {


struct PointerModelDefault : public PointerModel
{
    std::size_t sizeof_pointer() override { return sizeof(MemPtr); }
    void on_memblock_allocated(MemPtr) override {}
    void on_memblock_released(MemPtr) override {}
    MemPtr read_pointer(MemPtr const from) override { return *(MemPtr*)from; }
    void write_pointer(MemPtr const to, MemPtr const ptr) override { *(MemPtr*)to = ptr; }
    void read_shift_and_write(MemPtr const to, MemPtr const from, std::int64_t const shift) override { *(MemPtr*)to = *(MemPtr*)from + shift; }
};


}

#endif
