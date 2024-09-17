#include <sala/memblock.hpp>
#include <utility/invariants.hpp>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

namespace sala::detail {


MemBlockData::MemBlockData(PointerModel* const pointer_model, std::size_t const num_bytes, std::uint8_t const init_value)
    : pointer_model_{ pointer_model }
    , bytes{ new std::uint8_t[num_bytes] }
    , count_{ num_bytes }
{
    pointer_model_->on_memblock_allocated(bytes, count_);
}


MemBlockData::~MemBlockData()
{
    pointer_model_->on_memblock_released(bytes, count_);
}


}

namespace sala {


MemBlock::MemBlock()
    : data_{}
{}


MemBlock::MemBlock(PointerModel* const pointer_model, std::size_t const num_bytes, std::uint8_t const init_value)
    : data_{ std::make_shared<detail::MemBlockData>(pointer_model, num_bytes, init_value) }
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


}
