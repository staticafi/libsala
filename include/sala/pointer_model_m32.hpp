#ifndef SALA_POINTER_MODEL_M32_HPP_INCLUDED
#   define SALA_POINTER_MODEL_M32_HPP_INCLUDED

#   include <sala/pointer_model.hpp>
#   include <unordered_map>
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

        PointerAndBlock const& find(MemPtr32bit ptr32) const { return lo2hi_.at(ptr32); }

    private:
        Map32to64 lo2hi_;
        Map64to32 hi2lo_;
        MemBlockMap blocks_;
    };

    PtrMap32bit ptr_map_{};
};


}

#endif
