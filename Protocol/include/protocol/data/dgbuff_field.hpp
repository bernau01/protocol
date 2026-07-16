
#ifndef PROTOCOL_UTILS_BUFFFIELD_HPP
#define PROTOCOL_UTILS_BUFFFIELD_HPP

#include "../common.hpp"
#include "../status.hpp"
#include "dgbuff.hpp"

#include <cstring>
#include <type_traits>

namespace protocol
{

template <typename Type, size_t Msb, size_t Lsb = 0>
struct MaskBit
{
    static_assert((Msb < sizeof(Type) * 8) && (Lsb < sizeof(Type) * 8) && (Msb >= Lsb),
                  "Invalid MSB and LSB for Mask");

private:
    template <size_t F, size_t L, bool>
    [[nodiscard]] static inline constexpr 
    Type getMask();

    template <size_t F, size_t L, typename std::enable_if_t<(F < L), bool> = true>
    [[nodiscard]] static inline constexpr 
    Type getMask()
    {
        return (1 << F) | getMask<F + 1, L>();
    }

    template <size_t F, size_t L, typename std::enable_if_t<(F >= L), bool> = true>
    [[nodiscard]] static inline constexpr 
    Type getMask()
    {
        return 0;
    }

public:
    static inline constexpr Type value = getMask<Lsb, Msb>();
};

template <typename Type, size_t Offset, Type MaskVal = 0, size_t BitShift = 0,
          typename = std::enable_if_t<std::is_integral_v<Type> && !(MaskVal == 0 && BitShift != 0), bool>>
struct BuffField
{
    template <typename T = Type>
    [[nodiscard]] static inline constexpr 
    RetStatus<T> get(const DgBuff& buff)
    {
        auto status = Status::OK;
        Type data;
        if constexpr(sizeof(Type) == 1) {
            tie(status, data) = buff.get(Offset);
        }
        else {
            tie(status, data) = buff.getCopy<Type>(Offset);
        }

        if(status != Status::OK) {
            return RetStatus(status, T{});
        }

        if constexpr(MaskVal != 0) {
            data = GET_MASKED(data, MaskVal, BitShift);
        }

        return RetStatus(Status::OK, static_cast<T>(data));
    }

    /// @brief  Unsafe get the field value
    /// @note   Please don't use this unless the offset and size has been checked and bounded in the buffer 
    // template <typename T = Type, typename = std::enable_if_t<(Offset % sizeof(Type)) == 0, bool>>
    template <typename T = Type>
    [[nodiscard]] static inline constexpr 
    T unsafeGet(const DgBuff& buff)
    {
        auto ptr = buff.ptr(Offset);
        // Type data = *(reinterpret_cast<Type*>(ptr));
        Type data = buff.unsafeGetCopy<Type>(Offset);

        if constexpr(MaskVal != 0) {
            data = getMasked(data, MaskVal, BitShift);
        }

        return static_cast<T>(data);
    }

    template <typename T = Type, typename = std::enable_if_t<std::is_integral_v<T>, bool>>
    [[nodiscard]] static inline constexpr 
    Status set(T value, DgBuff& buff)
    {
        auto status = Status::OK;

        Type data;

        if constexpr(MaskVal == 0) {
            data = value << BitShift;
        }
        else {
            tie(status, data) = buff.getCopy<Type>(Offset);
            if(status != Status::OK) {
                return status;
            }
            setMasked(data, static_cast<Type>(value), MaskVal, BitShift);
            status = buff.setCopy<Type>(Offset, data);
        }

        return status;
    }

    /// @brief  Unsafe set the field value
    /// @note   Please don't use this unless the offset and size has been checked and bounded in the buffer 
    // template <typename T = Type, typename = std::enable_if_t<std::is_integral_v<T> && (Offset % sizeof(Type) == 0), bool>>
    template <typename T = Type>
    static inline constexpr 
    void unsafeSet(T value, DgBuff& buff)
    {
        Type data = buff.unsafeGetCopy<Type>(Offset);

        if constexpr(MaskVal != 0) {
            setMasked(data, static_cast<Type>(value), MaskVal, BitShift);
        }
        else {
            data = static_cast<Type>(value);
        }

        buff.unsafeSetCopy(data, Offset);
    }
};

template <typename Type, size_t Offset, size_t MSB = (sizeof(Type) * 8 - 1), size_t LSB = 0>
using BuffFieldM = BuffField<Type, Offset, MaskBit<Type, MSB, LSB>::value, LSB>;

} // namespace protocol

#endif // PROTOCOL_UTILS_BUFFFIELD_HPP