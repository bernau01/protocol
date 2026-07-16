#ifndef PROTOCOL_NODE_SET_HPP
#define PROTOCOL_NODE_SET_HPP

#include "../utils/func_set.hpp"
#include "node_packet.hpp"

namespace protocol
{

namespace node_set
{
    template <auto instance>
    static inline constexpr 
    Status getterFun(NodePacket& packet) 
    {
        return instance->inputPacket(packet);
    }

    template <auto instance>
    static inline constexpr 
    Status setterFun(NodePacket& packet) 
    {
        return instance->outputPacket(packet);
    }
} // namespace node_set

struct NodeSetTag {};

template <auto instance>
struct NodeSet : public NodeSetTag
{
    using InstanceType = typename std::remove_pointer_t<decltype(instance)>;
    using GetterFunc = StaticFuncSet<node_set::getterFun<instance>, instance, NodePacket&>;
    using SetterFunc = StaticFuncSet<node_set::setterFun<instance>, instance, NodePacket&>;

    GetterFunc getter;
    SetterFunc setter;
};

} // namespace protocol

#endif // PROTOCOL_NODE_SET_HPP