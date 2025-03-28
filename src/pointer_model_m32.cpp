#include <sala/pointer_model_m32.hpp>
#include <utility/hash_combine.hpp>
#include <utility/invariants.hpp>
#include <utility/assumptions.hpp>

namespace sala {


std::size_t PointerModelM32::sizeof_pointer()
{
    return sizeof(MemPtr32bit);
}


void PointerModelM32::on_memblock_allocated(MemPtr const block_ptr)
{
    ptr_map_.insert(block_ptr, block_ptr);
}


void PointerModelM32::on_memblock_released(MemPtr const block_ptr)
{
    ptr_map_.erase_block(block_ptr);
}


MemPtr PointerModelM32::read_pointer(MemPtr const from)
{
    MemPtr32bit const ptr{ *(MemPtr32bit*)from };
    return ptr == nullptr_32bit ? nullptr : ptr_map_.at(ptr);
}


void PointerModelM32::write_pointer(MemPtr const to, MemPtr const ptr)
{
    *(MemPtr32bit*)to = ptr_map_.insert(ptr, ptr);
}


void PointerModelM32::read_shift_and_write(MemPtr const to, MemPtr const from, std::int64_t const shift)
{
    auto const& ptr_and_block{ ptr_map_.find(*(MemPtr32bit*)from) };
    *(MemPtr32bit*)to = ptr_map_.insert(ptr_and_block.pointer + shift, ptr_and_block.block);
}


void PointerModelM32::write_uint8_as_pointer(MemPtr const to, std::uint8_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32::write_uint16_as_pointer(MemPtr const to, std::uint16_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32::write_uint32_as_pointer(MemPtr const to, std::uint32_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32::write_uint64_as_pointer(MemPtr const to, std::uint64_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32::write_pointer_as_uint8(MemPtr const to, MemPtr const ptr)
{
    *(std::uint8_t*)to = (std::uint8_t)ptr_map_.insert(ptr, ptr);
}


void PointerModelM32::write_pointer_as_uint16(MemPtr const to, MemPtr const ptr)
{
    *(std::uint16_t*)to = (std::uint16_t)ptr_map_.insert(ptr, ptr);
}


void PointerModelM32::write_pointer_as_uint32(MemPtr const to, MemPtr const ptr)
{
    *(std::uint32_t*)to = (std::uint32_t)ptr_map_.insert(ptr, ptr);
}


void PointerModelM32::write_pointer_as_uint64(MemPtr const to, MemPtr const ptr)
{
    *(std::uint64_t*)to = (std::uint64_t)ptr_map_.insert(ptr, ptr);
}


PointerModelM32::MemPtr32bit PointerModelM32::hash_ptr64(MemPtr const ptr)
{
    MemPtr32bit result{ *(MemPtr32bit*)&ptr };
    ::hash_combine(result, *((MemPtr32bit*)&ptr + 1U));
    return result;
}


PointerModelM32::PtrMap32bit::PtrMap32bit()
    : lo2hi_{ { (MemPtr32bit)0, { nullptr, nullptr } } }
    , hi2lo_{ { nullptr, (MemPtr32bit)0 } }
    , blocks_{ { (MemPtr)nullptr, {} } }
{}


PointerModelM32::MemPtr32bit PointerModelM32::PtrMap32bit::insert(MemPtr const ptr, MemPtr const block_ptr)
{
    auto const it = hi2lo_.find(ptr);
    if (it != hi2lo_.end())
        return it->second;

    blocks_.insert({ block_ptr, {} }).first->second.push_back(ptr);

    auto ptr32{ hash_ptr64(ptr) };
    for ( ; true; ++ptr32)
    {
        auto const it_and_state = lo2hi_.insert({ ptr32, { ptr, block_ptr } });
        if (it_and_state.second)
        {
            hi2lo_.insert({ ptr, ptr32 });
            break;
        }
    }
    return ptr32;
}


void PointerModelM32::PtrMap32bit::erase(MemPtr const ptr)
{
    auto const it = hi2lo_.find(ptr);
    if (it != hi2lo_.end())
    {
        lo2hi_.erase(it->second);
        hi2lo_.erase(it);
    }
}


void PointerModelM32::PtrMap32bit::erase_block(MemPtr const block_ptr)
{
    auto const it = blocks_.find(block_ptr);
    if (it == blocks_.end())
        return;
    for (auto const ptr : it->second)
        erase(ptr);
    blocks_.erase(it);
}


bool PointerModelM32_SegmentOffset::has_free_segments(std::size_t const count) const
{
    return count <= std::numeric_limits<Segment>::max() - fresh_segment + released_segments.size();
}


std::size_t PointerModelM32_SegmentOffset::sizeof_pointer()
{
    return sizeof(Segment) + sizeof(Offset);
}


void PointerModelM32_SegmentOffset::on_memblock_allocated(MemPtr const block_ptr)
{
    if (released_segments.empty())
    {
        ASSUMPTION(fresh_segment < std::numeric_limits<Segment>::max());
        seg2ptr.insert({ fresh_segment, block_ptr });
        ptr2seg.insert({ block_ptr, fresh_segment });
        ++fresh_segment;
    }
    else
    {
        seg2ptr.insert({ released_segments.back(), block_ptr });
        ptr2seg.insert({ block_ptr, released_segments.back() });
        released_segments.pop_back();
    }
}


void PointerModelM32_SegmentOffset::on_memblock_released(MemPtr const block_ptr)
{
    auto const it = ptr2seg.find(block_ptr);
    if (it != ptr2seg.end())
    {
        released_segments.push_back(it->second);
        seg2ptr.erase(released_segments.back());
        ptr2seg.erase(block_ptr);
    }
}


MemPtr PointerModelM32_SegmentOffset::read_pointer(MemPtr const from)
{
    MemPtr32bit const ptr32bit{ *(MemPtr32bit*)from };
    if (ptr32bit == nullptr_32bit)
        return nullptr;
    Segment const segment{ (Segment)(ptr32bit >> (8U * sizeof(Offset))) };
    auto const it = seg2ptr.find(segment);
    if (it == seg2ptr.end())
        return nullptr;
    Offset const offset{ (Offset)(ptr32bit & std::numeric_limits<Offset>::max()) };
    return it->second + offset;
}


void PointerModelM32_SegmentOffset::write_pointer(MemPtr const to, MemPtr const ptr)
{
    ASSUMPTION(!ptr2seg.empty());
    auto it = ptr2seg.lower_bound(ptr);
    while (it == ptr2seg.end() || (it != ptr2seg.begin() && ptr < it->first))
        it = std::prev(it);
    if (ptr < it->first)
        *(MemPtr32bit*)to = nullptr_32bit;
    else
    {
        std::int64_t const delta = ptr - it->first;
        if (delta > std::numeric_limits<Offset>::max())
            *(MemPtr32bit*)to = nullptr_32bit;
        else
        {
            MemPtr32bit const ptr32bit{ (((MemPtr32bit)it->second) << (8U * sizeof(Offset))) | (MemPtr32bit)delta };
            *(MemPtr32bit*)to = ptr32bit;
        }
    }
}


void PointerModelM32_SegmentOffset::read_shift_and_write(MemPtr const to, MemPtr const from, std::int64_t const shift)
{
    write_pointer(to, read_pointer(from) + shift);
}


void PointerModelM32_SegmentOffset::write_uint8_as_pointer(MemPtr const to, std::uint8_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32_SegmentOffset::write_uint16_as_pointer(MemPtr const to, std::uint16_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32_SegmentOffset::write_uint32_as_pointer(MemPtr const to, std::uint32_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32_SegmentOffset::write_uint64_as_pointer(MemPtr const to, std::uint64_t const int_ptr)
{
    MemPtr32bit ptr32bit{ (MemPtr32bit)int_ptr };
    write_pointer(to, read_pointer((MemPtr)&ptr32bit));
}


void PointerModelM32_SegmentOffset::write_pointer_as_uint8(MemPtr const to, MemPtr const ptr)
{
    MemPtr32bit ptr32bit;
    write_pointer((MemPtr)&ptr32bit, ptr);
    *(std::uint8_t*)to = (std::uint8_t)ptr32bit;
}


void PointerModelM32_SegmentOffset::write_pointer_as_uint16(MemPtr const to, MemPtr const ptr)
{
    MemPtr32bit ptr32bit;
    write_pointer((MemPtr)&ptr32bit, ptr);
    *(std::uint16_t*)to = (std::uint16_t)ptr32bit;
}


void PointerModelM32_SegmentOffset::write_pointer_as_uint32(MemPtr const to, MemPtr const ptr)
{
    MemPtr32bit ptr32bit;
    write_pointer((MemPtr)&ptr32bit, ptr);
    *(std::uint32_t*)to = (std::uint32_t)ptr32bit;
}


void PointerModelM32_SegmentOffset::write_pointer_as_uint64(MemPtr const to, MemPtr const ptr)
{
    MemPtr32bit ptr32bit;
    write_pointer((MemPtr)&ptr32bit, ptr);
    *(std::uint64_t*)to = (std::uint64_t)ptr32bit;
}


}
