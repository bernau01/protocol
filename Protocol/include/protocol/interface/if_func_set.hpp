#ifndef PROTOCOL_INTERFACE_SET_HPP
#define PROTOCOL_INTERFACE_SET_HPP

#include "../utils/func_set.hpp"
#include "../packet/packet.hpp"

namespace protocol
{

namespace func_set
{
    template <auto instance>
    static inline constexpr Status sendToInterface(Packet& packet) 
    {
        return instance->fromDevice(packet);
    }
}

template <auto instance>
using InterfaceSet = StaticFuncSet<func_set::sendToInterface<instance>, instance, Packet&>;

}

#endif // PROTOCOL_INTERFACE_SET_HPP