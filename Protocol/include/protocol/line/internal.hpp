#ifndef PROTOCOL_LINE_INTERNAL
#define PROTOCOL_LINE_INTERNAL

#include "line.hpp"
#include "../node/node.hpp"
#include "../node/node_func_set.hpp"
#include "../status.hpp"

#include <type_traits>
#include <array>

namespace protocol
{

using NodeSetGetFunc = FuncPtr<Status, NodePacket&>;

struct NodeSetGetFuncSet
{
    NodeSetGetFunc getter;
    NodeSetGetFunc setter;
};

template <typename ...TNodes>
class InternalLine : public Line
{
private:
    static inline constexpr size_t node_size = sizeof...(TNodes);
    
    template <typename T>
    using IsNode = std::is_base_of<NodeTag, T>;

public:
    explicit InternalLine(const TNodes& ...nodes) : Line(), m_nodes{getSet(nodes)...} {}

    Status inputPacket(Packet &packet) override
    {
        NodePacket node_packet{packet};
        auto status = Status::OK;

        do {
            auto node_id = node_packet.getNodeId();
            if(node_id >= node_size) {
                status = Status::NotFound;
                node_packet.setReplayStatus(status);
                break;
            }

            auto fun = node_packet.isWrite() ? m_nodes[node_id].setter : m_nodes[node_id].getter;
            if(fun == nullptr) {
                status = Status::NotFound;
                node_packet.setReplayStatus(status);
                break;
            }

            status = fun(node_packet);

        } while(false); 
        
        this->toDevice(packet);

        return status;
    }

private:

    template <typename T>
    [[gnu::always_inline]] static inline constexpr 
    auto getSet(const T& func_set) 
    {
        static_assert(std::is_base_of_v<NodeSetTag, T>);
        static_assert(std::is_base_of_v<NodeTag, typename T::InstanceType>, "All instances must be derived from NodeTag");
        return NodeSetGetFuncSet{
            func_set.getter.callFunction,
            func_set.setter.callFunction
        };
    }

    template <typename ...TSetNodes>
    [[gnu::always_inline]] static inline constexpr 
    auto getFuncSetArray(const TSetNodes& ...func_set) 
    {
        return std::array<NodeSetGetFuncSet, node_size>{getSet(func_set)...};
    }


private:
    const std::array<NodeSetGetFuncSet, node_size> m_nodes;
};


}

#endif // PROTOCOL_LINE_INTERNAL