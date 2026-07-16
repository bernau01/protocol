///
/// @file   rx_parser.hpp

#ifndef PROTOCOL_ETH_RX_PARSER
#define PROTOCOL_ETH_RX_PARSER

#include "eth_packet.hpp"
#include "../packet/packet.hpp"
#include "protocol/packet/packet_field.hpp"

#include <cstring>
#include <limits>

namespace protocol::eth
{

class RxParser
{
public:
    explicit RxParser(DgBuff &packet_buff) 
        : m_packet(packet_buff) {}

    [[nodiscard]] inline
    Status check(uint16_t dev_sn, uint16_t dev_addr) 
    {
        if(!m_packet.isPreambleValid()) {
            return Status::Refused;
        }

        if(!m_packet.isBufferValid()) {
            return Status::Malformed;
        }

        if(m_packet.getDevSn() != dev_sn || m_packet.getDevAddr() != dev_addr) {
            return Status::Invalid;
        }

        m_unparse_pl_buff = m_packet.getPayloadBuff().getValue();
        m_seq = 0;
        m_num_packets = m_packet.getNodeLen();

        return Status::OK;
    }

    [[nodiscard]] inline constexpr
    uint16_t getPacketCounter() const
    {
        return m_packet.getCounter();
    }

    [[nodiscard]] inline
    RetStatus<protocol::Packet> getNextPacket()
    {
        if(m_seq >= m_num_packets) {
            return {Status::RunOut, protocol::Packet{}};
        }

        if(m_unparse_pl_buff.size() == 0) {
            return {Status::RunOut, protocol::Packet{}};
        }

        protocol::PacketField active_packet_field(m_unparse_pl_buff);
        auto ret = active_packet_field.parsePacket();
        if(!ret.isOk()) {
            return {ret.getStatus(), protocol::Packet{}};
        }

        if(ret.getValue().getFrameId() != m_seq) {
            return {Status::Malformed, protocol::Packet{}};
        }

        m_seq += 1;

        return ret;
    }

    [[nodiscard]] inline constexpr
    bool isEnd() const
    {
        return m_seq >= m_num_packets;
    }

    [[nodiscard]] inline
    RetStatus<uint8_t> getSpecialPacket() const
    {
        if(auto node_len = m_packet.getNodeLen(); node_len > 0x7F) {
            return {Status::OK, node_len};
        }
        return {Status::NotFound, 0};
    }

public:

private:
    PacketField m_packet;
    DgBuff m_unparse_pl_buff;
    size_t m_seq = 0;
    size_t m_num_packets;
};

} // namespace protocol::eth

#endif // PROTOCOL_ETH_RX_PARSER