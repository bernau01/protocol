#ifndef PROTOCOL_UART_LINE_HPP
#define PROTOCOL_UART_LINE_HPP

#include "../line/line.hpp"
#include "../data/dgbuff.hpp"
#include <limits>

namespace protocol
{

template <typename TUartDriver>
class UartLine : public Line
{
    static inline constexpr size_t max_uart_data_len = 64;
public:
    UartLine(TUartDriver& uart_driver) : m_uart_driver(uart_driver) {}

    Status inputPacket(Packet &packet) override
    {
        auto can_id = packet.getCommand();
        auto data_buff = packet.getDataBuff();
        auto data_len = packet.getDataLen();
        if(data_len > 8 || data_buff.size() < data_len) {
            return Status::Invalid;
        }
        m_uart_driver.transmit(data_buff.ptr(0), data_len);
        return Status::OK;
    }

protected:

    void receiveCallback(uint8_t* data, size_t len)
    {
        Packet packet;
        packet.setCommand(0);
        packet.setDataLen(len);
        packet.setDataBuff(DgBuff{data, len});
        packet.setInterfaceId(std::numeric_limits<uint8_t>::max());

        this->toDevice(packet);
    }

private:
    TUartDriver& m_uart_driver;
};

} // namespace protocol

#endif // PROTOCOL_UART_LINE_HPP