#ifndef PROTOCOL_NODE_HPP
#define PROTOCOL_NODE_HPP

#include "../common.hpp"
#include "../data/dgbuff.hpp"
#include "node_packet.hpp"

namespace protocol
{

class NodeTag {};

template <size_t seg_size>
class Node : public NodeTag
{
public:
    Node(size_t id) : m_id(id) {}

    [[nodiscard]]
    Status inputPacket(NodePacket &packet_inplace) 
    {
        size_t len_bytes = packet_inplace.getDataLen();
        if(!isFourBytes(len_bytes)) {
            packet_inplace.setReplayStatus(Status::Malformed);
            return Status::Malformed;
        }

        auto seg_id = packet_inplace.getSegId();
        if(seg_id > seg_size) {
            packet_inplace.setReplayStatus(Status::OutOfRange);
            return Status::OutOfRange;
        }

        auto data_buff = packet_inplace.getDataBuff();

        auto ret = data_buff.copyTo(const_cast<uint32_t*>(&m_seg[seg_id]), len_bytes, 0);

        if(ret == Status::OK) {
            packet_inplace.setReplayStatus(Status::OK);
        }
        else {
            packet_inplace.setReplayStatus(Status::Malformed);
        }

        return ret;
    }

    [[nodiscard]]
    Status outputPacket(NodePacket &packet_inplace) 
    {
        size_t len_bytes = packet_inplace.getDataLen();

        if(len_bytes > segToByte(seg_size) || !isFourBytes(len_bytes)) {
            packet_inplace.setReplayStatus(Status::Malformed);
            return Status::Malformed;
        }

        auto seg_id = packet_inplace.getSegId();
        if(seg_id > seg_size) {
            packet_inplace.setReplayStatus(Status::OutOfRange);
            return Status::OutOfRange;
        }

        uint32_t* seg_ptr = const_cast<uint32_t*>(&m_seg[seg_id]);
        uint8_t* ptr = reinterpret_cast<uint8_t*>(seg_ptr);
        auto data_buff = DgBuff{ptr, len_bytes};

        packet_inplace.setReplayBuff(data_buff);

        return Status::OK;
    }

    RetStatus<uint32_t> getSegData(size_t seg_id)
    {
        if(seg_id >= seg_size) {
            return RetStatus<uint32_t>(Status::OutOfRange, uint32_t{});
        }
        return RetStatus<uint32_t>(Status::OK, m_seg[seg_id]);
    }

    Status setSegData(size_t seg_id, uint32_t value)
    {
        if(seg_id >= seg_size) {
            return Status::OutOfRange;
        }
        m_seg[seg_id] = value;
        return Status::OK;
    }

private:
    [[nodiscard]] static inline constexpr
    bool isFourBytes(size_t len) 
    {
        return (len & 3) == 0;
    }

private:
    const size_t m_id;
    volatile uint32_t m_seg[seg_size];
};

} // namespace protocol

#endif // PROTOCOL_NODE_HPP