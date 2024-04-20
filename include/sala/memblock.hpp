#ifndef SALA_MEMBLOCK_HPP_INCLUDED
#   define SALA_MEMBLOCK_HPP_INCLUDED

#   include <sala/pointer_model.hpp>
#   include <vector>
#   include <unordered_map>
#   include <memory>
#   include <cstdint>

namespace sala {


struct MemBlock final
{
    MemBlock();
    MemBlock(PointerModel* pointer_model, std::size_t num_bytes, std::uint8_t init_value = 0xcd);

    MemPtr start() const { return data_->start(); }
    std::size_t count() const { return data_->count(); }

    std::size_t as_size() const;

    template<typename T>
    T read() const { return *(T*)start(); }

    template<>
    MemPtr read<MemPtr>() const { return data_->read_pointer(); }

    template<typename T>
    void write(T const value) const { *(T*)start() = value; }

    template<>
    void write<MemPtr>(MemPtr const ptr) const { data_->write_pointer(ptr); }

    template<>
    void write<std::nullptr_t>(std::nullptr_t const ptr) const { data_->write_pointer(ptr); }

    void write_shifted(MemPtr const from, std::int64_t const shift) const { data_->read_shift_and_write_pointer(from, shift); }

private:

    struct Data final
    {
        Data(PointerModel* pointer_model, std::size_t num_bytes, std::uint8_t init_value);
        ~Data();
        PointerModel* pointer_model() const { return pointer_model_; }
        MemPtr start() const { return bytes; }
        std::size_t count() const { return count_; }
        MemPtr read_pointer() const { return pointer_model()->read_pointer(start()); }
        void write_pointer(MemPtr const ptr) const { pointer_model()->write_pointer(start(), ptr); }
        void read_shift_and_write_pointer(MemPtr const from, std::int64_t const shift) const { pointer_model()->read_shift_and_write(start(), from, shift); }
    private:
        PointerModel* pointer_model_;
        std::uint8_t* bytes;
        std::size_t count_;
    };

    std::shared_ptr<Data> data_;
};


}

#endif
