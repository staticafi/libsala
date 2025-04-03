#ifndef SALA_POINTER_MODEL_M32_HPP_INCLUDED
#   define SALA_POINTER_MODEL_M32_HPP_INCLUDED

#   include <sala/pointer_model.hpp>
#   include <unordered_map>
#   include <map>
#   include <vector>

namespace sala {


struct PointerModelM32 : public PointerModel
{
    std::size_t sizeof_pointer() override;
    void on_memblock_allocated(MemPtr block_ptr) override;
    void on_memblock_released(MemPtr block_ptr) override;
    MemPtr read_pointer(MemPtr from) override;
    void write_pointer(MemPtr to, MemPtr ptr) override;
    void read_shift_and_write(MemPtr to, MemPtr from, std::int64_t shift) override;
    void write_uint8_as_pointer(MemPtr to, std::uint8_t int_ptr) override;
    void write_uint16_as_pointer(MemPtr to, std::uint16_t int_ptr) override;
    void write_uint32_as_pointer(MemPtr to, std::uint32_t int_ptr) override;
    void write_uint64_as_pointer(MemPtr to, std::uint64_t int_ptr) override;
    void write_pointer_as_uint8(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint16(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint32(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint64(MemPtr to, MemPtr ptr) override;

private:

    using MemPtr32bit = std::uint32_t;

    static MemPtr32bit constexpr nullptr_32bit = 0U;
    static MemPtr32bit hash_ptr64(MemPtr ptr);

    struct PtrMap32bit
    {
        struct PointerAndBlock
        {
            MemPtr pointer;
            MemPtr block;
        };

        using Map32to64 = std::unordered_map<MemPtr32bit, PointerAndBlock>;
        using Map64to32 = std::unordered_map<MemPtr, MemPtr32bit>;
        using MemBlockMap = std::unordered_map<MemPtr, std::vector<MemPtr> >;

        PtrMap32bit();

        MemPtr32bit insert(MemPtr ptr, MemPtr block_ptr);
        void erase(MemPtr ptr);
        void erase_block(MemPtr block_ptr);

        MemPtr32bit at(MemPtr const ptr) const { return hi2lo_.at(ptr); }
        MemPtr at(MemPtr32bit const ptr32) const { return find(ptr32).pointer; }

        PointerAndBlock const& find(MemPtr32bit const ptr32) const {
            auto const it = lo2hi_.find(ptr32);
            return it == lo2hi_.end() ? lo2hi_.at(0U) : it->second; 
        }

    private:
        Map32to64 lo2hi_;
        Map64to32 hi2lo_;
        MemBlockMap blocks_;
    };

    PtrMap32bit ptr_map_{};
};


struct PointerModelM32_SegmentOffset : public PointerModel
{
    bool has_free_segments(std::size_t count) const override;
    std::size_t sizeof_pointer() override;
    void on_memblock_allocated(MemPtr block_ptr) override;
    void on_memblock_released(MemPtr block_ptr) override;
    MemPtr read_pointer(MemPtr from) override;
    void write_pointer(MemPtr to, MemPtr ptr) override;
    void read_shift_and_write(MemPtr to, MemPtr from, std::int64_t shift) override;
    void write_uint8_as_pointer(MemPtr to, std::uint8_t int_ptr) override;
    void write_uint16_as_pointer(MemPtr to, std::uint16_t int_ptr) override;
    void write_uint32_as_pointer(MemPtr to, std::uint32_t int_ptr) override;
    void write_uint64_as_pointer(MemPtr to, std::uint64_t int_ptr) override;
    void write_pointer_as_uint8(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint16(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint32(MemPtr to, MemPtr ptr) override;
    void write_pointer_as_uint64(MemPtr to, MemPtr ptr) override;

private:

    using MemPtr32bit = std::uint32_t;
    static MemPtr32bit constexpr nullptr_32bit = 0U;

    using Segment = std::uint16_t;
    using Offset = std::uint16_t;

    std::map<MemPtr, Segment> ptr2seg{};
    std::unordered_map<Segment, MemPtr> seg2ptr{};
    std::vector<Segment> released_segments{};
    Segment fresh_segment{ 1U };
};


}

#endif
