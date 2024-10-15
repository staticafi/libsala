#ifndef SALA_POINTER_MODEL_HPP_INCLUDED
#   define SALA_POINTER_MODEL_HPP_INCLUDED

#   include <utility>
#   include <cstdint>

namespace sala {


using MemPtr = std::uint8_t*;


struct PointerModel
{
    virtual ~PointerModel() {}

    std::size_t num_allocated_bytes() const { return num_allocated_bytes_; }

    void on_memblock_allocated(MemPtr const block_ptr, std::size_t const num_bytes)
    { num_allocated_bytes_ += num_bytes; on_memblock_allocated(block_ptr); }

    void on_memblock_released(MemPtr block_ptr, std::size_t const num_bytes)
    { num_allocated_bytes_ -= num_bytes; on_memblock_released(block_ptr); }

    virtual std::size_t sizeof_pointer() = 0;
    virtual void on_memblock_allocated(MemPtr block_ptr) = 0;
    virtual void on_memblock_released(MemPtr block_ptr) = 0;
    virtual MemPtr read_pointer(MemPtr from) = 0;
    virtual void write_pointer(MemPtr to, MemPtr ptr) = 0;
    virtual void read_shift_and_write(MemPtr to, MemPtr from, std::int64_t shift) = 0;

private:
    std::size_t num_allocated_bytes_{ 0ULL };
};


}

#endif
