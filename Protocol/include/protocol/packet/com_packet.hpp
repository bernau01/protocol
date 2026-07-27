#ifndef PROTOCOL_COM_PACKET_HPP
#define PROTOCOL_COM_PACKET_HPP

#include "../data/dgbuff.hpp"
#include <string_view>

namespace protocol
{

struct Common
{
    static constexpr uint8_t max_non_valid = 0x7F;
    static constexpr uint8_t broadcast_addr = 0xFF;

    static constexpr size_t comm_header_size = 8;
    static constexpr size_t datagram_offset = comm_header_size;

    static constexpr size_t data_padding_byte = 4;

    static inline constexpr
    size_t getInBuffSize(size_t size)
    {
        constexpr uint8_t unit_min_one = Common::data_padding_byte - 1;
        uint8_t tailed_len = (size + unit_min_one) & ~unit_min_one;
        return tailed_len;
    }
};

enum class ComPacketType : uint8_t
{
    Echo            = 0x80,
    Heartbeat       = 0x81,
    ReqConnect      = 0x82,
    RespConnect     = 0x83,
    ReqDisconnect   = 0x84,
    RespDisconnect  = 0x85,
    CheckHost       = 0x86,

    Busy            = 0xF0,
    InvalidCmd      = 0xFE,
    IllegalCmd      = 0xFF
};

inline constexpr 
std::string_view getComPacketTypeName(ComPacketType type)
{
    switch(type) {
        case ComPacketType::Echo: return "Echo";
        case ComPacketType::Heartbeat: return "Heartbeat";
        case ComPacketType::ReqConnect: return "ReqConnect";
        case ComPacketType::RespConnect: return "RespConnect";
        case ComPacketType::ReqDisconnect: return "ReqDisconnect";
        case ComPacketType::RespDisconnect: return "RespDisconnect";
        case ComPacketType::Busy: return "Busy";
        case ComPacketType::InvalidCmd: return "InvalidCmd";
        case ComPacketType::IllegalCmd: return "IllegalCmd";
        default: 
            if(static_cast<uint8_t>(type) <= Common::max_non_valid) {
                return "Data Packet";
            }
        return "Unknown";
    }
}

class Port;

struct ComPacket
{
    Port *port = nullptr;
    DgBuff data_buff;
    
    union 
    {
        uint8_t num_of_nodes = 0;
        ComPacketType type;
    };
    uint8_t sn = 0;
    uint8_t addr = 0;
    uint8_t counter = 0;

    bool isCommand() const
    {
        return num_of_nodes > 0x7F;
    }
};

} // namespace protocol    

#endif // PROTOCOL_COM_PACKET_HPP