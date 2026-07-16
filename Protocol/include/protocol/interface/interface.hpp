#ifndef PROTOCOL_INTERFACE_HPP
#define PROTOCOL_INTERFACE_HPP

#include "../device/dev_child.hpp"
#include "if_func_set.hpp"

namespace protocol
{

class Interface : public DevChild
{
public:
    explicit Interface() : DevChild() {}
    virtual ~Interface() = default;
    virtual Status fromDevice(Packet &packet)
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }
};


class NullInterface : public Interface
{
private:
    NullInterface() = default;
public:
    Status fromDevice(Packet &packet) override
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }

    static NullInterface instance;
};

}

#endif // PROTOCOL_INTERFACE_HPP