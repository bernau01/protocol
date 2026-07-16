#ifndef PROTOCOL_PACKET_HPP
#define PROTOCOL_PACKET_HPP

#include "../common.hpp"
#include "../data/dgbuff.hpp"

namespace protocol {

struct PacketCommon
{
    static constexpr size_t header_size_std = 4;
    static constexpr size_t header_size_ext = 8;
};

enum class DataType : bool
{
    Data = false,
    Command = true
};

enum class CmdType : bool
{
    Standard = false,
    Extended = true
};

struct Header 
{
    uint8_t frame_id = 0;
    uint8_t line_id = 0;
    CmdType cmd_type = CmdType::Standard;
    DataType data_type = DataType::Data;
    uint8_t data_len = 0;
    uint8_t reserved0_;
    uint16_t reserved1_;
    uint32_t command = 0;
};

class Packet
{
public:
    explicit Packet() {}

    Packet(const Packet &other) = default;
    Packet &operator=(const Packet &other) = default;
    Packet(Packet &&other) = default;
    Packet &operator=(Packet &&other) = default;

    uint8_t getFrameId() const { return m_header.frame_id; }
    uint8_t getLineId() const { return m_header.line_id; }
    bool isExtended() const { return m_header.cmd_type == CmdType::Extended; }
    bool isCommand() const { return m_header.data_type == DataType::Command; }
    uint8_t getDataLen() const { return m_header.data_len; }
    uint32_t getCommand() const { return m_header.command; }
    uint8_t getInterfaceId() const { return m_interface_id; }

    void setFrameId(uint8_t frame_id) { m_header.frame_id = frame_id; }
    void setLineId(uint8_t line_id) { m_header.line_id = line_id; }
    void setCmdType(CmdType ext) { m_header.cmd_type = ext; }
    void setDataType(DataType cmd) { m_header.data_type = cmd; }
    void setDataLen(uint8_t len) { m_header.data_len = len; }
    void setCommand(uint32_t cmd) { m_header.command = cmd; }
    void setDataBuff(const DgBuff &buff) { m_data_buff = buff; }
    void setInterfaceId(uint8_t id) { m_interface_id = id; }

    Header& getHeader() { return m_header; }
    const Header& getHeader() const { return m_header; }

    DgBuff& getDataBuff() { return m_data_buff; }
    const DgBuff& getDataBuff() const { return m_data_buff; }

    size_t getHeaderSize() const
    { 
        return (isExtended()) ? PacketCommon::header_size_ext : PacketCommon::header_size_std; 
    }
 
    size_t getPacketSize() const
    {
        return getHeaderSize() + getDataLen();
    }

protected:
    DgBuff m_data_buff;
    Header m_header;
    uint8_t m_interface_id = 0;
};

} // namespace protocol

#endif // PROTOCOL_PACKET_HPP