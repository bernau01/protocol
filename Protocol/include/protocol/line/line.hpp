#ifndef PROTOCOL_LINE_HPP
#define PROTOCOL_LINE_HPP

#include "../device/dev_child.hpp"
#include "line_func_set.hpp"

namespace protocol
{

class Line : public DevChild
{
public:
    explicit Line() : DevChild() {}
    virtual ~Line() = default;

    virtual Status inputPacket(Packet &packet)
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }
};


class NullLine final : public Line
{
private:
    NullLine() = default;
public:
    Status inputPacket(Packet &packet) override
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }

    static NullLine instance;
};

}

#endif // PROTOCOL_LINE_HPP