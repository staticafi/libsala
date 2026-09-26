#include <sala/memblock.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace sala::detail {


MemBlockData::MemBlockData(PointerModel* const pointer_model, MemPtr const start_addr, std::size_t const num_bytes, std::uint8_t const init_value)
    : pointer_model_{ pointer_model }
    , bytes{ start_addr == nullptr ? new std::uint8_t[num_bytes] : start_addr }
    , count_{ num_bytes }
    , is_memory_owner{ start_addr == nullptr }
{
    if (is_memory_owner)
        pointer_model_->on_memblock_allocated(bytes, count_);
}


MemBlockData::~MemBlockData()
{
    if (is_memory_owner)
    {
        pointer_model_->on_memblock_released(bytes, count_);
        delete [] bytes;
    }
}


}

namespace sala {


MemBlock::MemBlock()
    : data_{}
{}


MemBlock::MemBlock(PointerModel* const pointer_model, MemPtr const start_addr, std::size_t const num_bytes, std::uint8_t const init_value)
    : data_{ std::make_shared<detail::MemBlockData>(pointer_model, start_addr, num_bytes, init_value) }
{
    std::memset(start(), init_value, count());
}


std::size_t MemBlock::as_size() const
{
    switch (count())
    {
    case 1ULL: return (std::size_t)*start();
    case 2ULL: return (std::size_t)*(std::uint16_t*)start();
    case 4ULL: return (std::size_t)*(std::uint32_t*)start();
    case 8ULL: return (std::size_t)*(std::uint64_t*)start();
    default: UNREACHABLE(); return 0ULL;
    }
}


std::int64_t MemBlock::as_shift() const
{
    switch (count())
    {
    case 1ULL: return (std::int64_t)*start();
    case 2ULL: return (std::int64_t)*(std::int16_t*)start();
    case 4ULL: return (std::int64_t)*(std::int32_t*)start();
    case 8ULL: return (std::int64_t)*(std::int64_t*)start();
    default: UNREACHABLE(); return 0ULL;
    }
}


}
