#ifndef PROTOCOL_ETH_TX_COMPOSER
#define PROTOCOL_ETH_TX_COMPOSER

#include "../common.hpp"
#include "../data/dgbuff.hpp"
#include "../packet/packet.hpp"
#include "../utils/time.hpp"
#include "eth_packet.hpp"
#include "protocol/packet/packet_field.hpp"

#include <atomic>

namespace protocol::eth
{

class TxComposer
{
private:
    class Guard
    {
    public:
        Guard(std::atomic_bool &lock) : lock(lock) {}
        [[nodiscard]] bool tryLock() { return !lock.exchange(true); }
        void unlock() { lock.exchange(false); }
        ~Guard() { unlock(); }
    private:
        std::atomic_bool &lock;
    };
    
public:
    TxComposer(DgBuff &packet_buff) : m_packet(packet_buff)
    {}

    [[nodiscard]]
    Status init(size_t dev_sn, size_t dev_addr)
    {
        // Check validity of the buffer
        if(!m_packet.isBufferValid()) {
            return Status::OutOfRange;
        }

        // Write static data to the buffer
        m_packet.writePreamble();
        m_packet.setDevSn(dev_sn);
        m_packet.setDevAddr(dev_addr);

        // Prepare the buffers
        m_payload_buff = m_packet.getPayloadBuff().getValue();
        m_remain_buff = m_payload_buff;
        m_space_count = 0;
        return Status::OK;
    }

    [[nodiscard]]
    Status place(protocol::Packet &packet)
    {
        // Lock the buffer to prevent concurrent access
        Guard lock_guard(m_lock);
        if (UNLIKELY(!lock_guard.tryLock())) {
            return Status::Busy;
        }

        auto status = Status::OK;

        // Get the required size for the packet and check if there is enough space in the remaining buffer
        auto packet_size = packet.getPacketSize();
        auto requred_size = Common::getInBuffSize(packet_size);
        if(m_remain_buff.size() < requred_size) {
            return Status::RunOut;
        }

        protocol::PacketField packet_field(m_remain_buff);

        // Set the sequence number
        packet.setFrameId(m_space_count);

        // Compose the packet into the remaining buffer
        status = packet_field.composePacket(packet);
        if(status != Status::OK) {
            return status;
        }

        // Advance the remaining buffer by used size
        status = m_remain_buff.advance(requred_size);
        if(status != Status::OK) {
            return status;
        }

        m_space_count += 1;

        return Status::OK;
    }

    [[nodiscard]]
    Status place(const DgBuff &data)
    {
        Guard lock_guard(m_lock);
        if (UNLIKELY(!lock_guard.tryLock())) {
            return Status::Busy;
        }

        auto status = Status::OK;

        auto requred_size = Common::getInBuffSize(data.size());
        if(m_remain_buff.size() < requred_size) {
            return Status::RunOut;
        }

        status = data.copyTo(m_remain_buff, data.size(), 0);
        if(status != Status::OK) {
            return status;
        }

        status = m_remain_buff.advance(requred_size);
        if(status != Status::OK) {
            return status;
        }

        m_space_count += 1;

        return Status::OK;
    }

    [[nodiscard]]
    Status startTransmission()
    {
        if(UNLIKELY(!tryLock())) {
            return Status::Busy;
        }

        m_packet.setNodeLen(m_space_count);
        m_packet.setCounter(m_packet_count);
        m_packet.setTimeStamp(utils::getCurrentTimestamp());

        return Status::OK;
    }

    [[nodiscard]] inline 
    DgBuff getFinalBuff() const
    {
        auto buff = m_packet.getBuff().unsafeSub(0, getUsedBuffLen());
        return buff;
    }

    void endTransmissionAndReset()
    {
        m_remain_buff = m_payload_buff;
        m_space_count = 0;
        m_packet_count += 1;
        unlock();
    }

    [[nodiscard]]
    size_t getPayloadLen() const
    {        
        return m_payload_buff.size() - m_remain_buff.size();
    }

    [[nodiscard]]
    size_t getUsedBuffLen() const
    {        
        return getPayloadLen() + Common::header_size;
    }

    [[nodiscard]]
    RetStatus<DgBuff> getBuff()
    {        
        return m_payload_buff.sub(0, getPayloadLen());
    }

    [[nodiscard]]
    size_t getPacketCount()
    {        
        return m_space_count;
    }

    [[nodiscard]]
    Status reset()
    {
        Guard lock_guard(m_lock);
        if (UNLIKELY(!lock_guard.tryLock())) {
            return Status::Busy;
        }

        m_remain_buff = m_payload_buff;
        m_space_count = 0;
        
        return Status::OK;
    }

    [[nodiscard]]
    bool isBusy()
    {
        return isLocked();
    }

    [[nodiscard]]
    bool isEmpty() const
    {
        return m_space_count == 0;
    }

private:

    [[nodiscard]]
    bool tryLock()
    {
        return !m_lock.exchange(true);
    }

    void unlock()
    {
        m_lock = false;
    }

    bool isLocked() const
    {
        if(m_lock) {
            return true;
        }
        return false;
    }

private:
    PacketField m_packet;
    DgBuff m_payload_buff;
    DgBuff m_remain_buff;

    size_t m_space_count{0};
    size_t m_packet_count{0};

    std::atomic_bool m_lock{false};
};

} // namespace protocol::eth

#endif // PROTOCOL_ETH_TX_COMPOSER