#ifndef PROTOCOL_TX_COMPOSER
#define PROTOCOL_TX_COMPOSER

#include "../common.hpp"
#include "../data/dgbuff.hpp"
#include "../packet/packet.hpp"
#include "../packet/packet_field.hpp"
#include "../packet/com_packet.hpp"
#include "../utils/lock.hpp"

namespace protocol
{

class TxComposer
{
private:
    
public:
    TxComposer(DgBuff &buff) : m_buff(buff)
    {}

    TxComposer() : m_buff(DgBuff{})
    {}

    Status setBuff(const DgBuff &buff)
    {
        m_buff = buff;
        return reset();
    }

    Status setMaxPacketCount(size_t count)
    {
        m_max_packet_count = count;
        return Status::OK;
    }

    [[nodiscard]]
    Status place(const Packet &packet)
    {
        if(m_space_count >= m_max_packet_count) {
            return Status::RunOut;
        }

        // Lock the buffer to prevent concurrent access
        LockGuard lock_guard(m_lock);
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

        packet_field.setFrameId(m_space_count);

        m_space_count += 1;

        return Status::OK;
    }

    [[nodiscard]]
    Status place(const DgBuff &data)
    {
        LockGuard lock_guard(m_lock);
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
        if(UNLIKELY(!m_lock.tryLock())) {
            return Status::Busy;
        }

        return Status::OK;
    }

    [[nodiscard]] inline 
    DgBuff getPayloadBuff() const
    {
        auto buff = m_buff.unsafeSub(0, getPayloadLen());
        return buff;
    }

    void endTransmissionAndReset()
    {
        m_remain_buff = m_buff;
        m_space_count = 0;
        m_lock.unlock();
    }

    [[nodiscard]]
    size_t getPayloadLen() const
    {        
        return m_buff.size() - m_remain_buff.size();
    }

    [[nodiscard]]
    RetStatus<DgBuff> getBuff()
    {        
        return m_buff.sub(0, getPayloadLen());
    }

    [[nodiscard]]
    size_t getPacketCount()
    {        
        return m_space_count;
    }

    [[nodiscard]]
    Status reset()
    {
        LockGuard lock_guard(m_lock);
        if (UNLIKELY(!lock_guard.tryLock())) {
            return Status::Busy;
        }

        m_remain_buff = m_buff;
        m_space_count = 0;
        
        return Status::OK;
    }

    [[nodiscard]]
    bool isBusy()
    {
        return m_lock.isLocked();
    }

    [[nodiscard]]
    bool isEmpty() const
    {
        return m_space_count == 0;
    }

private:
    DgBuff m_buff;
    DgBuff m_remain_buff;

    size_t m_space_count{0};
    size_t m_max_packet_count{0};

    Lock m_lock;
};

} // namespace protocol::eth

#endif // PROTOCOL_ETH_TX_COMPOSER