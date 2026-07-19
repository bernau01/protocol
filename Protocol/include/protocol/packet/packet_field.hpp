#ifndef PROTOCOL_PACKET_FIELD_HPP
#define PROTOCOL_PACKET_FIELD_HPP

#include "../common.hpp"
#include "../data/dgbuff.hpp"
#include "../data/dgbuff_field.hpp"
#include "packet.hpp"

namespace protocol {

class PacketField
{
    static constexpr size_t frame_id_offset = 0;
    static constexpr size_t frame_id_bit_shift = 4;
    static constexpr uint8_t frame_id_mask = 0x0F << frame_id_bit_shift;

    static constexpr size_t line_id_offset = 0;
    static constexpr size_t line_id_bit_shift = 0;
    static constexpr uint8_t line_id_mask = 0x0F << line_id_bit_shift;

    static constexpr size_t ext_offset = 1;
    static constexpr size_t ext_bit_shift = 7;
    static constexpr uint8_t ext_mask = 0x01 << ext_bit_shift;

    static constexpr size_t dc_offset = 1;
    static constexpr size_t dc_bit_shift = 6;
    static constexpr uint8_t dc_mask = 0x01 << dc_bit_shift;

    static constexpr size_t data_len_offset = 1;
    static constexpr size_t data_len_bit_shift = 0;
    static constexpr uint8_t data_len_mask = 0x3F << data_len_bit_shift;

    static constexpr size_t command_offset = 2;
    static constexpr size_t command_std_size = 2;
    static constexpr size_t command_ext_size = 4;

public:
    using FrameIdField      = BuffField<uint8_t, frame_id_offset, frame_id_mask, frame_id_bit_shift>;
    using LineIdField       = BuffField<uint8_t, line_id_offset, line_id_mask, line_id_bit_shift>;
    using ExtField          = BuffField<uint8_t, ext_offset, ext_mask, ext_bit_shift>;
    using DcField           = BuffField<uint8_t, dc_offset, dc_mask, dc_bit_shift>;
    using DataLenField      = BuffField<uint8_t, data_len_offset, data_len_mask, data_len_bit_shift>;
    using CommandStdField   = BuffField<uint16_t, command_offset>;
    using CommandExtField   = BuffField<uint32_t, command_offset>;

public:
    explicit PacketField(const DgBuff &buff) : m_buff(buff) {}

    uint8_t getFrameId() const { return FrameIdField::unsafeGet(m_buff); }
    uint8_t getLineId() const { return LineIdField::unsafeGet(m_buff); }
    bool isExtended() const { return ExtField::unsafeGet(m_buff); }
    bool isCommand() const { return DcField::unsafeGet(m_buff); }
    uint8_t getDataLen() const { return DataLenField::unsafeGet(m_buff); }
    uint32_t getCommandStd() const { return CommandStdField::unsafeGet(m_buff); }
    uint32_t getCommandExt() const { return CommandExtField::unsafeGet(m_buff); }

    void setFrameId(uint8_t frame_id) { FrameIdField::unsafeSet(frame_id, m_buff); }
    void setLineId(uint8_t line_id) { LineIdField::unsafeSet(line_id, m_buff); }
    void setExtended(bool ext) { ExtField::unsafeSet(ext, m_buff); }
    void setSendCmd(bool cmd) { DcField::unsafeSet(cmd, m_buff); }
    void setDataLen(uint8_t len) { DataLenField::unsafeSet(len, m_buff); }
    void setCommandStd(uint32_t cmd) { CommandStdField::unsafeSet(cmd, m_buff); }
    void setCommandExt(uint32_t cmd) { CommandExtField::unsafeSet(cmd, m_buff); }

    /// @brief Encode header properties to datagram/buffer.
    ///        the target buffer should have enough space for header and data, otherwise it will return error status.
    /// @param dest_buff 
    /// @return Status status of operation
    Status composePacket(const Packet &packet)
    {
        const auto& header = packet.getHeader();

        auto header_len = packet.getHeaderSize();
        if(m_buff.size() < header_len) {
            return Status::RunOut;
        }

        auto data_len = packet.isCommand() ? 0 : header.data_len;
        if(m_buff.size() < header_len + data_len) {
            return Status::RunOut;
        }
        if(packet.getDataBuff().size() < data_len) {
            return Status::Invalid;
        }

        FrameIdField::unsafeSet(header.frame_id, m_buff);
        LineIdField::unsafeSet(header.line_id, m_buff);
        ExtField::unsafeSet(packet.isExtended(), m_buff);
        DcField::unsafeSet(packet.isCommand(), m_buff);
        DataLenField::unsafeSet(header.data_len, m_buff);

        if(packet.isExtended()) {
            CommandExtField::unsafeSet(header.command, m_buff);
        } else {
            CommandStdField::unsafeSet(header.command, m_buff);
        }

        if(data_len > 0 && packet.getDataBuff().ptr(0) != m_buff.ptr(header_len)) {
            auto ret = m_buff.copyFrom(packet.getDataBuff(), data_len, header_len);
            if(ret != Status::OK) {
                return ret;
            }
        }

        return Status::OK;
    }

    /// @brief Decode/parse header properties from datagram/buffer
    /// @param dest_buff 
    /// @return Status status of operation
    RetStatus<Packet> parsePacket() const
    {
        // Initial check for header size. Standard header is the minimum size for now
        // header size and data length will be checked again after parsing header
        if(m_buff.size() < PacketCommon::header_size_std) {
            return RetStatus<Packet>{Status::Malformed, Packet{}};
        }

        // Below code is safe to access the buffer data until header_size_std bytes, as we have already checked the size

        // Create a temporary packet to hold the parsed header
        Packet packet;
        Header& header = packet.getHeader();

        // Get fixed data field data from the buffer
        // We should assign this data to header first, as other fields depend on these values
        header.cmd_type = isExtended() ? CmdType::Extended : CmdType::Standard;
        header.data_type = isCommand() ? DataType::Command : DataType::Data;
        header.data_len = getDataLen();

        const size_t required_size = packet.getPacketSize();
        if(m_buff.size() < required_size) {
            return RetStatus<Packet>{Status::Malformed, Packet{}};
        }

        // After this point, we can safely access the buffer data until required_size bytes

        // Get remaining fields from the buffer
        header.command = packet.isExtended() ? getCommandExt() : getCommandStd();
        header.frame_id = getFrameId();
        header.line_id = getLineId();

        const size_t header_size = packet.getHeaderSize();
        packet.getDataBuff() = m_buff.unsafeSub(header_size, header.data_len);

        return RetStatus<Packet>{Status::OK, packet};
    }

protected:
    DgBuff m_buff;
};

} // namespace protocol

#endif // PROTOCOL_PACKET_FIELD_HPP