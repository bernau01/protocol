#ifndef PROTOCOL_LINE_HPP
#define PROTOCOL_LINE_HPP

#include "../device/dev_child.hpp"
#include "../utils/list.hpp"

namespace protocol
{

class Device;

class Line
{
public:
    explicit Line() {}
    virtual ~Line() = default;

    virtual Status inputPacket(Packet &packet)
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }

protected:
    [[nodiscard]] inline
    Device* getDevice() const { return m_device; }

private:
    void setDevice(Device* device) { m_device = device; }

    Device* m_device = nullptr;
    friend class Device;
};

}

#endif // PROTOCOL_LINE_HPP