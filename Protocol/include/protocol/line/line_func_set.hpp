#ifndef PROTOCOL_LINE_FUNC_SET_HPP
#define PROTOCOL_LINE_FUNC_SET_HPP

#include "../utils/func_set.hpp"
#include "../packet/packet.hpp"

namespace protocol
{

namespace line_set
{
    template <auto instance>
    static inline constexpr Status inputPacket(Packet& packet) 
    {
        return instance->inputPacket(packet);
    }
}

template <auto instance>
using LineSet = StaticFuncSet<line_set::inputPacket<instance>, instance, protocol::Packet&>;

}

#endif // PROTOCOL_LINE_FUNC_SET_HPP