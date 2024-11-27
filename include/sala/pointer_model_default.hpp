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
    void write_uint8_as_pointer(MemPtr const to, std::uint8_t const int_ptr) override { *(MemPtr*)to = (MemPtr)(std::uint64_t)int_ptr; }
    void write_uint16_as_pointer(MemPtr const to, std::uint16_t const int_ptr) override { *(MemPtr*)to = (MemPtr)(std::uint64_t)int_ptr; }
    void write_uint32_as_pointer(MemPtr const to, std::uint32_t const int_ptr) override { *(MemPtr*)to = (MemPtr)(std::uint64_t)int_ptr; }
    void write_uint64_as_pointer(MemPtr const to, std::uint64_t const int_ptr) override { *(MemPtr*)to = (MemPtr)(std::uint64_t)int_ptr; }
    void write_pointer_as_uint8(MemPtr const to, MemPtr const ptr) override { *(std::uint8_t*)to = (std::uint8_t)(std::uint64_t)ptr; }
    void write_pointer_as_uint16(MemPtr const to, MemPtr const ptr) override { *(std::uint16_t*)to = (std::uint16_t)(std::uint64_t)ptr; }
    void write_pointer_as_uint32(MemPtr const to, MemPtr const ptr) override { *(std::uint32_t*)to = (std::uint32_t)(std::uint64_t)ptr; }
    void write_pointer_as_uint64(MemPtr const to, MemPtr const ptr) override { *(std::uint64_t*)to = (std::uint64_t)(std::uint64_t)ptr; }
};


}

#endif
