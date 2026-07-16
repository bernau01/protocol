#ifndef PROTOCOL_NODE_PACKET_HPP
#define PROTOCOL_NODE_PACKET_HPP

#include "../packet/packet.hpp"
#include "../data/dgbuff.hpp"

namespace protocol
{

class NodePacket
{
    static constexpr size_t node_id_bit_shift = 0;
    static constexpr uint16_t node_id_mask = 0x00FF << node_id_bit_shift;

    static constexpr size_t seg_idx_bit_shift = 8;
    static constexpr uint16_t seg_idx_mask = 0x00FF << seg_idx_bit_shift;
public:
    NodePacket(Packet& packet) : m_packet(packet) {}

    ~NodePacket()
    {
        if(static_cast<void*>(&m_respone_code) == m_packet.getDataBuff().ptr(0)) {
            m_packet.setDataBuff(DgBuff{});
            m_respone_code = 0;
        }
    }

    inline
    uint8_t getNodeId() const
    {
        auto command = m_packet.getCommand();
        return GET_MASKED(command, node_id_mask, node_id_bit_shift);
    }

    inline
    uint8_t getSegId() const
    {
        auto command = m_packet.getCommand();
        return GET_MASKED(command, seg_idx_mask, seg_idx_bit_shift);
    }

    inline
    size_t getDataLen() const
    {
        return m_packet.getDataLen();
    }

    inline
    DgBuff getDataBuff() const
    {
        return m_packet.getDataBuff();
    }

    inline
    void setDataBuff(DgBuff& buff)
    {
        m_packet.setDataBuff(buff);
    }

    void setReplay()
    {
        m_packet.setDataType(DataType::Command);
    }

    void setReplayBuff(DgBuff& buff)
    {
        m_packet.setDataType(DataType::Command);
        m_packet.setDataBuff(buff);
        m_packet.setDataLen(buff.size());
    }

    void setReplayStatus(Status status)
    {
        m_respone_code = static_cast<uint32_t>(status);
        const size_t len_bytes = sizeof(m_respone_code);
        m_packet.setDataType(DataType::Command);
        m_packet.setDataBuff(DgBuff(reinterpret_cast<uint8_t*>(&m_respone_code), len_bytes));
        m_packet.setDataLen(len_bytes);
    }

    void setReplayEmpty()
    {
        m_packet.setDataType(DataType::Command);
        m_packet.setDataBuff(DgBuff(nullptr, 0));
        m_packet.setDataLen(0);
    }

    inline
    bool isWrite() const
    {
        return !m_packet.isCommand();
    }

private:
    Packet& m_packet;
    uint32_t m_respone_code = 0;
};

} // namespace protocol

#endif // PROTOCOL_NODE_PACKET_HPP