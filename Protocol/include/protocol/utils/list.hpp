#ifndef PROTOCOL_SPAN_HPP
#define PROTOCOL_SPAN_HPP

#include <cstddef>
#include "../status.hpp"

namespace protocol
{

template <typename T>
class SpanList
{
protected:
    SpanList(T *lines, size_t size) 
        : lines(lines), size(size) 
    {}

public:
    SpanList(const SpanList&) = default;
    SpanList& operator=(const SpanList&) = delete;
    SpanList(SpanList&&) = default;
    SpanList& operator=(SpanList&&) = delete;

    [[nodiscard]] inline
    size_t getSize() const
    {
        return size;
    }

    [[nodiscard]] inline
    RetStatus<T> get(size_t index) const
    {
        if(index >= size) {
            return {Status::NotFound, T{}};
        }
        return {Status::OK, lines[index]};
    }

    [[nodiscard]] inline
    T& operator[](size_t index) const
    {
        return lines[index];
    }

private:
    T *lines;
    size_t size;
};

template <typename T, size_t Size>
class StaticList : public SpanList<T>
{
public:
    template <typename ...TLines>
    explicit StaticList(const TLines&... lines) 
        : SpanList<T>(m_lines, Size), m_lines{lines...}
    {}

private:
    T m_lines[Size];
};

template <typename ...TLines>
StaticList(const TLines& ...lines) -> StaticList<std::common_type_t<TLines...>, sizeof...(TLines)>;

}

#endif // PROTOCOL_SPAN_HPP