#ifndef SALA_POINTER_MODEL_HPP_INCLUDED
#   define SALA_POINTER_MODEL_HPP_INCLUDED

#   include <cstdint>

namespace sala {


using MemPtr = std::uint8_t*;


struct PointerModel
{
    virtual ~PointerModel() {}
    virtual std::size_t sizeof_pointer() = 0;
    virtual void on_memblock_allocated(MemPtr block_ptr) = 0;
    virtual void on_memblock_released(MemPtr block_ptr) = 0;
    virtual MemPtr read_pointer(MemPtr from) = 0;
    virtual void write_pointer(MemPtr to, MemPtr ptr) = 0;
    virtual void read_shift_and_write(MemPtr to, MemPtr from, std::int64_t shift) = 0;
};


}

#endif
