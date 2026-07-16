#ifndef PROTOCOL_CAN_LINE_HPP
#define PROTOCOL_CAN_LINE_HPP

#include "../line/line.hpp"
#include "../data/dgbuff.hpp"
#include <limits>

namespace protocol
{

template <typename TCanDriver>
class CanLine : public Line
{
    static inline constexpr size_t max_can_data_len = 8;
public:
    CanLine(TCanDriver& can_driver) : m_can_driver(can_driver) {}

    Status inputPacket(Packet &packet) override
    {
        auto can_id = packet.getCommand();
        auto data_buff = packet.getDataBuff();
        auto data_len = packet.getDataLen();
        if(data_len > max_can_data_len || data_buff.size() < data_len) {
            return Status::Invalid;
        }
        uint32_t flag = (TCanDriver::efe_flag * packet.isExtended()) | (TCanDriver::rtr_flag * packet.isCommand()); 
        send(can_id, flag, data_buff.ptr(0), data_len);
        return Status::OK;
    }

protected:
    void send(uint32_t can_id, uint32_t flag, const uint8_t* data, size_t len)
    {
        m_can_driver.send(can_id, flag, data, len);
    }

    void receiveCallback(uint32_t can_id, uint32_t flag, uint8_t* data, size_t len)
    {
        Packet packet;
        packet.setCommand(can_id);
        packet.setCmdType((flag & TCanDriver::efe_flag) ? CmdType::Extended : CmdType::Standard);
        packet.setDataType((flag & TCanDriver::rtr_flag) ? DataType::Command : DataType::Data);
        packet.setDataLen(len);
        packet.setDataBuff(DgBuff{data, len});
        packet.setInterfaceId(std::numeric_limits<uint8_t>::max());

        this->toDevice(packet);
    }

private:
    TCanDriver& m_can_driver;
};

} // namespace protocol

#endif // PROTOCOL_CAN_LINE_HPP