#include <sala/pointer_model_m32.hpp>
#include <utility/hash_combine.hpp>

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


}
