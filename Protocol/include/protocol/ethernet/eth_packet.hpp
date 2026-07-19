///
/// @file   eth_packet.hpp

#ifndef PROTOCOL_ETH_PACKET
#define PROTOCOL_ETH_PACKET

#include "../data/dgbuff.hpp"
#include "../data/dgbuff_field.hpp"
#include "../status.hpp"
#include "../packet/com_packet.hpp"

#include <cstring>

namespace protocol::eth
{

// struct Common 
// {
//     static constexpr size_t data_padding_byte = 4;
//     static constexpr size_t header_size = 16;
//     static constexpr size_t payload_offset = 16;
//     static constexpr size_t max_datagram_size = 1024;

//     static inline constexpr
//     size_t getInBuffSize(size_t size)
//     {
//         constexpr uint8_t unit_min_one = Common::data_padding_byte - 1;
//         uint8_t tailed_len = (size + unit_min_one) & ~unit_min_one;
//         return tailed_len;
//     }
// };

struct Header
{
    uint8_t version = 1;
    uint8_t node_len = 0;
    uint16_t dev_sn = 0;
    uint16_t dev_addr = 0;
    uint16_t counter = 0;
    uint16_t _reserved1 = 0;
    uint32_t timestamp = 0;
};

class PacketField
{
    static constexpr size_t preamble_offset = 0;
    static constexpr size_t preamble_size = 4;
    static constexpr uint32_t preamble_value = 0x01'73'74'69; // "its\01" in little-endian

    static constexpr size_t non_offset = 4;
    static constexpr size_t non_size = sizeof(uint8_t);

    static constexpr size_t dev_sn_offset = 5;
    static constexpr size_t dev_sn_size = sizeof(uint8_t);

    static constexpr size_t dev_addr_offset = 6;
    static constexpr size_t dev_addr_size = sizeof(uint8_t);

    static constexpr size_t counter_offset = 7;
    static constexpr size_t counter_size = sizeof(uint8_t);

    using PreambleField     = BuffFieldM<uint32_t, preamble_offset>;
    using DevSnField        = BuffField<uint8_t, dev_sn_offset>;
    using DevAddrField      = BuffField<uint8_t, dev_addr_offset>;
    using CounterField      = BuffField<uint8_t, counter_offset>;
    using NONField          = BuffField<uint8_t , non_offset>;

public:
    explicit PacketField(DgBuff &buff) : m_buff(buff) {}

    [[nodiscard]]
    bool isPreambleValid() const
    {
        // Check header size
        if(m_buff.size() < Common::comm_header_size) {
            return false;
        }

        // Check "its" preamble
        // Take pointer of the first 4 bytes and compare with preamble value
        auto prea = PreambleField::unsafeGet(m_buff);
        if(prea != preamble_value) {
            return false;
        }

#if 0
        // Check "its" preamble
        auto buff_arr = m_buff.ptr(0);
        if(memcmp(buff_arr + preamble_offset, "its", preamble_size) != 0) {
            return false;
        }

        // Check version. it must be one
        if(VersionField::unsafeGet(m_buff) != 1) {
            return false;
        } 
#endif
        return true;
    }

    bool isBufferValid() const
    {
        if(m_buff.size() < Common::comm_header_size) {
            return false;
        }
        return true;
    }

    /// @brief Check the header of the packet and extract packet length and timestamp.
    ///        return immediately if get an error, for supporting fast sequential
    ///        filtering (used by LwIP)
    /// @param buff
    /// @return Status
    [[nodiscard]]
    RetStatus<Header> getHeader() const
    {
        // if the validity and bound checking is passed, code below are safe

        Header temp;

        // Get device serial number
        temp.dev_sn = DevSnField::unsafeGet(m_buff);

        // Get device address
        temp.dev_addr = DevAddrField::unsafeGet(m_buff);

        // Get counter
        temp.counter = CounterField::unsafeGet(m_buff);

        // Get node length
        temp.node_len = NONField::unsafeGet(m_buff);

        return {Status::OK, temp};
    }

    [[nodiscard]] inline constexpr
    uint16_t getDevSn() const
    {
        return DevSnField::unsafeGet(m_buff);
    }

    [[nodiscard]] inline constexpr
    uint16_t getDevAddr() const
    {
        return DevAddrField::unsafeGet(m_buff);
    }

    [[nodiscard]] inline constexpr
    uint16_t getCounter() const
    {
        return CounterField::unsafeGet(m_buff);
    }

    [[nodiscard]] inline constexpr
    uint8_t getNumOfNodes() const
    {
        return NONField::unsafeGet(m_buff);
    }

    [[nodiscard]]
    RetStatus<DgBuff> getPayloadBuff() const
    {
        auto ret = m_buff.sub(Common::comm_header_size);
        return ret;
    }

    [[nodiscard]]
    const DgBuff& getBuff() const
    {
        return m_buff;
    } 

    void writePreamble() 
    {
        PreambleField::unsafeSet(preamble_value, m_buff);
    }

    void setDevSn(uint16_t dev_sn)
    {
        DevSnField::unsafeSet(dev_sn, m_buff);
    }

    void setDevAddr(uint16_t dev_addr) 
    {
        DevAddrField::unsafeSet(dev_addr, m_buff);
    }

    void setCounter(uint16_t counter) 
    {
        CounterField::unsafeSet(counter, m_buff);
    }

    void setNumOfNodes(size_t node_len) 
    {
        NONField::unsafeSet(node_len, m_buff);
    }

    [[nodiscard]]
    Status setHeader(const Header& header) 
    {
        if(auto ret = isBufferValid()) {
            return Status::Invalid;
        }
        
        // if the bound checking is passed, code below are safe
        PreambleField::unsafeSet(preamble_value, m_buff);

        DevSnField::unsafeSet(header.dev_sn, m_buff);
        DevAddrField::unsafeSet(header.dev_addr, m_buff);
        CounterField::unsafeSet(header.counter, m_buff);
        NONField::unsafeSet(header.node_len, m_buff);

        return Status::OK;
    }

private:
    DgBuff m_buff;
};

}

#endif // PROTOCOL_ETH_PACKET 