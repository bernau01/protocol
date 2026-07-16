/// @file common.hpp
/// @brief Common utilities and type definitions for the protocol.
/// @author Zain

#ifndef __COMMON_HPP__
#define __COMMON_HPP__

#include <cstddef>
#include <cstdint>

#ifdef __GNUC__
#define EXPECT(x, v)    __builtin_expect(x, v)
#define LIKELY(x)       EXPECT(!!(x), 1)
#define UNLIKELY(x)     EXPECT(!!(x), 0)
#else
#define LIKELY(x)       x
#define UNLIKELY(x)     x
#define EXPECT(x, v)    x
#endif

namespace protocol
{

using byte_t = uint8_t;
using float32_t = float;
using float64_t = double;

#define GET_MASKED(DATA, MASK, BITSHIFT) ((DATA & MASK) >> BITSHIFT)
#define SET_MASKED(DEST, DATA, MASK, BITSHIFT)                                                     \
    do {                                                                                           \
        DEST = (((DATA) << BITSHIFT) & MASK);                                                      \
    } while(0)

inline constexpr size_t unit_size = sizeof(uint32_t);

template <typename T>
static inline constexpr 
T segToByte(T seg)
{
    return seg * sizeof(uint32_t);
}

template <typename T>
static inline constexpr
T getMasked(T data, T mask, size_t bitshift)
{
    return (data & mask) >> bitshift;
}

template <typename T>
static inline constexpr
void setMasked(T& dest, T data, T mask, size_t bitshift)
{
    dest &= ~mask;
    dest = ((data << bitshift) & mask);
}

template <typename T>
static inline constexpr
void orMasked(T& dest, T data, T mask, size_t bitshift)
{
    dest |= ((data << bitshift) & mask);
}

} // namespace protocol

#endif // __COMMON_HPP__