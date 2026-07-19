///
/// @file   rx_parser.hpp

#ifndef PROTOCOL_RX_PARSER
#define PROTOCOL_RX_PARSER

#include "../packet/packet.hpp"
#include "protocol/packet/packet_field.hpp"

#include <cstring>

namespace protocol
{

class RxParser
{
public:
    explicit RxParser(const DgBuff &datagram_buff, size_t num_packets) 
        : m_unparse_pl_buff(datagram_buff), m_num_packets(num_packets) {}

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

    [[nodiscard]] inline constexpr
    bool isRemaining() const
    {
        return (m_num_packets - m_seq) != 0;
    }

private:
    DgBuff m_unparse_pl_buff;
    size_t m_seq = 0;
    const size_t m_num_packets;
};

} // namespace protocol

#endif // PROTOCOL_ETH_RX_PARSER