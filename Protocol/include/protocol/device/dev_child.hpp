#ifndef PROTOCOL_DEVICE_CHILD_HPP
#define PROTOCOL_DEVICE_CHILD_HPP

#include "dev_func_set.hpp"

namespace protocol
{

class DevChild
{
public:
    void setDevice(const RouteFuncSet& dev_func) 
    { 
        if(m_dev_funcset.isOk()) {
            return;
        }
        m_dev_funcset = dev_func; 
    }

protected:
    explicit DevChild() {}

    Status toDevice(Packet& packet) 
    {
        if(not m_dev_funcset.isOk()) {
            return Status::NotInitialized;
        }
        return m_dev_funcset.callFunction(packet);
    }

private:
    RouteFuncSet m_dev_funcset;
};

}

#endif // PROTOCOL_DEVICE_CHILD_HPP