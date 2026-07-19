
#ifndef PROTOCOL_UTILS_DGBUFF_HPP
#define PROTOCOL_UTILS_DGBUFF_HPP

#include "../common.hpp"
#include "../status.hpp"

#include <algorithm>
#include <cstring>
#include <type_traits>

namespace protocol {

class DgBuff 
{
public:
    inline constexpr
    DgBuff(uint8_t* const buff, size_t size) 
        : buff_(buff), size_(size) {};
        
    inline constexpr
    DgBuff() : buff_(nullptr), size_(0) {};
    
    DgBuff(const DgBuff& other) = default;
    DgBuff& operator=(const DgBuff& other) = default;
    
    DgBuff(DgBuff&& other) = default;
    DgBuff& operator=(DgBuff&& other) = default;

    virtual ~DgBuff() 
    {
        size_ = 0;
        buff_ = nullptr;
    }

    [[nodiscard]] inline
    RetStatus<DgBuff> sub(size_t offset, size_t size) const 
    {
        if (offset + size > size_) {
            return RetStatus<DgBuff>(Status::OutOfRange, DgBuff());
        }
        return RetStatus<DgBuff>(Status::OK, DgBuff(buff_ + offset, size));
    }

    [[nodiscard]] inline
    RetStatus<DgBuff> sub(size_t offset) const 
    {
        if (offset > size_) {
            return RetStatus<DgBuff>(Status::OutOfRange, DgBuff());
        }
        return RetStatus<DgBuff>(Status::OK, DgBuff(buff_ + offset, size_ - offset));
    }

    [[nodiscard]] inline
    DgBuff unsafeSub(size_t offset, size_t size) const 
    {
        return DgBuff(buff_ + offset, size);
    }

    [[nodiscard]] inline
    DgBuff unsafeSub(size_t offset) const 
    {
        offset = std::min(offset, size_);
        return DgBuff(buff_ + offset, size_ - offset);
    }
    
    [[nodiscard]] inline constexpr
    uint8_t* ptr(size_t index) const 
    { 
        if(index >= size_) {
            return nullptr; // Out of bounds
        }
        return buff_ + index; 
    }

    [[nodiscard]] inline constexpr
    size_t size() const {return size_;}

    Status advance(size_t size) 
    {
        if(size > size_) 
            return Status::OutOfRange;
        size_ -= size;
        buff_ += size;
        return Status::OK;
    }

    [[nodiscard]] inline constexpr
    uint8_t unsafeGet(size_t index) 
    {
        return buff_[index];
    }

    [[nodiscard]] inline constexpr
    uint8_t& operator[](size_t index) 
    {
        return buff_[index];
    }

    [[nodiscard]] inline constexpr
    const uint8_t& operator[](size_t index) const 
    {
        return buff_[index];
    }

    [[nodiscard]] inline
    RetStatus<uint8_t> get(size_t index) const
    {
        if((index) > size_) {
            return RetStatus(Status::OutOfRange, uint8_t());
        }
        return RetStatus(Status::OK, *(buff_+static_cast<size_t>(index)));
    }

    template <typename T>
    [[nodiscard]] inline
    RetStatus<T> getCopy(size_t offset)
    {
        static_assert(!std::is_pointer_v<T>, "Template type T must not be a pointer type");
        static_assert(std::is_trivially_copyable_v<T>, "Template type T must be trivially copyable");

        if((offset + sizeof(T)) > size_) {
            return {Status::OutOfRange, T()};
        }

        T value = 0;
        std::memcpy(&value, buff_ + offset, sizeof(T));
        return {Status::OK, value};
    }

    template <typename T>
    [[nodiscard]] inline
    T unsafeGetCopy(size_t offset) const
    {
        static_assert(!std::is_pointer_v<T>, "Template type T must not be a pointer type");
        static_assert(std::is_trivially_copyable_v<T>, "Template type T must be trivially copyable");

        T value = 0;
        std::memcpy(&value, buff_ + offset, sizeof(T));
        return value;
    }

    inline constexpr
    Status copyTo(void* dest, size_t length, size_t offset) const
    {
        if((offset + length) > size()) {
            return Status::OutOfRange;
        }

        std::memcpy(dest, buff_ + offset, length);
        return Status::OK;
    }

    inline constexpr
    Status copyTo(DgBuff& dest, size_t length, size_t offset) const
    {
        if((offset + length) > size() || length > dest.size()) {
            return Status::OutOfRange;
        }

        std::memcpy(dest.ptr(0), buff_ + offset, length);
        return Status::OK;
    }

    inline constexpr
    Status set(size_t index, uint8_t value)
    {
        if((index + sizeof(uint8_t)) > size()) {
            return Status::OutOfRange;
        }

        buff_[index] = value;
        return Status::OK;
    }

    template <typename T>
    inline constexpr
    Status setCopy(const T& value, size_t offset)
    {
        static_assert(!std::is_pointer_v<T>, "Template type T must not be a pointer type");

        if((offset + sizeof(T)) > size()) {
            return Status::OutOfRange;
        }

        std::memcpy(buff_ + offset, &value, sizeof(T));
        return Status::OK;
    }

    template <typename T>
    inline constexpr
    void unsafeSetCopy(const T& value, size_t offset)
    {
        static_assert(!std::is_pointer_v<T>, "Template type T must not be a pointer type");
        std::memcpy(buff_ + offset, &value, sizeof(T));
    }

    inline constexpr
    Status copyFrom(const void* src, size_t length, size_t offset)
    {
        if((offset + length) > size()) {
            return Status::OutOfRange;
        }

        std::memcpy(buff_ + offset, src, length);
        return Status::OK;
    }

    inline constexpr
    Status copyFrom(const DgBuff& src, size_t length, size_t offset)
    {
        if((offset + length) > size() || length > src.size()) {
            return Status::OutOfRange;
        }

        std::memcpy(buff_ + offset, src.ptr(0), length);
        return Status::OK;
    }

    inline constexpr
    Status checkIsSub(const DgBuff& sub_buff, size_t offset) const
    {
        if(sub_buff.buff_ == buff_ + offset) {
            return Status::OK;
        }
        return Status::Invalid;
    }

private:
    uint8_t* buff_;
    size_t size_;
};

template <size_t N>
class StaticDgBuff : public DgBuff
{
public:
    inline constexpr
    StaticDgBuff() : DgBuff(m_data, N) {};

    DgBuff ref() const {
        return static_cast<DgBuff>(*this);
    }

private:
    uint8_t m_data[N] = {};
};

} // namespace protocol

#endif // PROTOCOL_UTILS_DGBUFF_HPP