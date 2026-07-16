#ifndef PROTOCOL_SESSION_HPP
#define PROTOCOL_SESSION_HPP

#include "../data/dgbuff.hpp"
#include "rx_parser.hpp"

namespace protocol
{

enum class SessionState
{
    Stopped,
    WaitForHost,
    Running
};

class Session
{
public:

    void start() 
    {
        m_state = SessionState::WaitForHost;
    }

    void inputDatagram(DgBuff &buff)
    {
        eth::RxParser parser{buff};

        if(auto ret = parser.check(0, 0); ret != Status::OK)
        {
            return;
        }

        auto packet_counter = parser.getPacketCounter();
        if(packet_counter != m_rx_counter)
        {
            // @todo: handle packet loss
            m_rx_counter = packet_counter;
        }

        switch(m_state)
        {
            case SessionState::Stopped:
                break;
            case SessionState::WaitForHost:
                break;
            case SessionState::Running:
                break;
            default:
                m_state = SessionState::Stopped;
                break;
        }
    }

    void checkHostHS(eth::RxParser &parser)
    {
        
    }

private:
    SessionState m_state = SessionState::Stopped;
    uint16_t m_rx_counter = 0;
};

} // namespace protocol

#endif // PROTOCOL_SESSION_HPP