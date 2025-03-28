#ifndef SALA_MEMBLOCK_HPP_INCLUDED
#   define SALA_MEMBLOCK_HPP_INCLUDED

#   include <sala/pointer_model.hpp>
#   include <vector>
#   include <unordered_map>
#   include <memory>
#   include <cstdint>

namespace sala::detail {


struct MemBlockData final
{
    MemBlockData(PointerModel* pointer_model, std::size_t num_bytes, std::uint8_t init_value);
    ~MemBlockData();
    PointerModel* pointer_model() const { return pointer_model_; }
    MemPtr start() const { return bytes; }
    std::size_t count() const { return count_; }
    MemPtr read_pointer() const { return pointer_model()->read_pointer(start()); }
    void write_pointer(MemPtr const ptr) const { pointer_model()->write_pointer(start(), ptr); }
    void write_pointer_from_offset(std::size_t const offset, MemPtr const ptr) const { pointer_model()->write_pointer(start() + offset, ptr); }
    void read_shift_and_write_pointer(MemPtr const from, std::int64_t const shift) const { pointer_model()->read_shift_and_write(start(), from, shift); }
    void write_uint8_as_pointer(std::uint8_t const int_ptr) const { pointer_model()->write_uint8_as_pointer(start(), int_ptr); }
    void write_uint16_as_pointer(std::uint16_t const int_ptr) const { pointer_model()->write_uint16_as_pointer(start(), int_ptr); }
    void write_uint32_as_pointer(std::uint32_t const int_ptr) const { pointer_model()->write_uint32_as_pointer(start(), int_ptr); }
    void write_uint64_as_pointer(std::uint64_t const int_ptr) const { pointer_model()->write_uint64_as_pointer(start(), int_ptr); }
    void write_pointer_as_uint8(MemPtr const ptr) const { pointer_model()->write_pointer_as_uint8(start(), ptr); }
    void write_pointer_as_uint16(MemPtr const ptr) const { pointer_model()->write_pointer_as_uint16(start(), ptr); }
    void write_pointer_as_uint32(MemPtr const ptr) const { pointer_model()->write_pointer_as_uint32(start(), ptr); }
    void write_pointer_as_uint64(MemPtr const ptr) const { pointer_model()->write_pointer_as_uint64(start(), ptr); }
private:
    PointerModel* pointer_model_;
    std::uint8_t* bytes;
    std::size_t count_;
};


template<typename T>
struct MemBlockDataReader
{ static inline T read(MemBlockData* const data) { return *(T*)data->start(); } };

template<typename T>
struct MemBlockDataReader<T*>
{ static inline T* read(MemBlockData* const data) { return (T*)data->read_pointer(); } };

template<>
struct MemBlockDataReader<MemPtr>
{ static inline MemPtr read(MemBlockData* const data) { return data->read_pointer(); } };


template<typename T>
struct MemBlockDataWriter
{ static inline void write(MemBlockData* const data, T const value) { *(T*)data->start() = value; } };

template<>
struct MemBlockDataWriter<MemPtr>
{ static inline void write(MemBlockData* const data, MemPtr const ptr) { data->write_pointer(ptr); } };

template<>
struct MemBlockDataWriter<std::nullptr_t>
{ static inline void write(MemBlockData* const data, std::nullptr_t const ptr) { data->write_pointer(ptr); } };


}

namespace sala {


struct MemBlock final
{
    MemBlock();
    MemBlock(PointerModel* pointer_model, std::size_t num_bytes, std::uint8_t init_value = 0xcd);

    MemPtr start() const { return data_->start(); }
    std::size_t count() const { return data_->count(); }

    std::size_t as_size() const;
    std::int64_t as_shift() const;

    template<typename T>
    T read() const { return detail::MemBlockDataReader<T>::read(data_.get()); }

    template<typename T>
    void write(T const value) const { detail::MemBlockDataWriter<T>::write(data_.get(), value); }
    void write_pointer_from_offset(std::size_t const offset, MemPtr const ptr) const { data_->write_pointer_from_offset(offset, ptr); }

    void write_shifted(MemPtr const from, std::int64_t const shift) const { data_->read_shift_and_write_pointer(from, shift); }
    void write_uint8_as_pointer(std::uint8_t const int_ptr) const { data_->write_uint8_as_pointer(int_ptr); }
    void write_uint16_as_pointer(std::uint16_t const int_ptr) const { data_->write_uint16_as_pointer(int_ptr); }
    void write_uint32_as_pointer(std::uint32_t const int_ptr) const { data_->write_uint32_as_pointer(int_ptr); }
    void write_uint64_as_pointer(std::uint64_t const int_ptr) const { data_->write_uint64_as_pointer(int_ptr); }
    void write_pointer_as_uint8(MemPtr const ptr) const { data_->write_pointer_as_uint8(ptr); }
    void write_pointer_as_uint16(MemPtr const ptr) const { data_->write_pointer_as_uint16(ptr); }
    void write_pointer_as_uint32(MemPtr const ptr) const { data_->write_pointer_as_uint32(ptr); }
    void write_pointer_as_uint64(MemPtr const ptr) const { data_->write_pointer_as_uint64(ptr); }

    PointerModel const* pointer_model() const { return data_->pointer_model(); }

private:

    std::shared_ptr<detail::MemBlockData> data_;
};


}

#endif
