#ifndef PROTOCOL_PORT_HPP
#define PROTOCOL_PORT_HPP

#include "../packet/com_packet.hpp"
#include "../status.hpp"

namespace protocol
{

class Device;

struct PortInfo
{
    size_t max_node = 0;
    uint8_t host_info[16];
};

class Port
{
public:
    Port(Device* device) : m_device(device) {}
    virtual ~Port() = default;
    virtual Status send(ComPacket &packet, const PortInfo &info) = 0;
    virtual PortInfo getCurrentHostInfo() = 0;
    virtual DgBuff allocateTxDataBuff() = 0;

protected:
    [[nodiscard]] inline
    Device* getDevice() const { return m_device; }

private:
    Device* m_device;
};

}

#endif // PROTOCOL_PORT_HPP