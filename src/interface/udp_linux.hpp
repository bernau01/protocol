#pragma once

#include <protocol/data/dgbuff.hpp>
#include <protocol/ethernet/eth_packet.hpp>
#include <protocol/ethernet/tx_composer.hpp>
#include <protocol/ethernet/rx_parser.hpp>
#include <protocol/interface/interface.hpp>
#include <protocol/line/line.hpp>
#include <stdexcept>

#include <bits/stdc++.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

namespace interface::eth
{
    
struct Config 
{
    char ip_addr[16];
    uint16_t port;
};

class UdpLinux final : public protocol::Interface 
{
public:
    private:
    static inline constexpr uint16_t max_retransmit_count = 5;

    enum class State : uint8_t
    {
        Uninit,
        WaitForHost,
        Connected,
        Terminated = std::numeric_limits<uint8_t>::max()
    };

    enum class TxState : uint16_t
    {
        Stopped,
        WaitForPacket,
        Transmitting,
        NeedToReset
    };
    
public:
    explicit UdpLinux(Config& config) : m_config(config) {}

    bool init()
    {
        m_sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if(m_sock_fd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        struct sockaddr_in this_addr;
        memset(&this_addr, 0, sizeof(this_addr)); 
        
        // Filling server information 
        this_addr.sin_family    = AF_INET; // IPv4 
        this_addr.sin_addr.s_addr = inet_addr(m_config.ip_addr); 
        this_addr.sin_port = htons(m_config.port); 

        if(bind(m_sock_fd, (const struct sockaddr *)&this_addr, sizeof(this_addr)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        setTimeout(std::chrono::milliseconds(5000));
        m_state = State::WaitForHost;

        return true;
    }

    protocol::Status fromDevice(protocol::Packet& packet)
    {
        if(auto ret = m_tx_composer.startTransmission(); ret != protocol::Status::OK) {
            return ret;
        }

        auto buff = m_tx_composer.getFinalBuff();

        auto ret = send(buff);
        
        if(!ret) {
            return protocol::Status::Invalid;
        }

        return protocol::Status::OK;
    }

    void callback(protocol::DgBuff& buff)
    {
        protocol::eth::RxParser parser{buff};
        if(auto ret = parser.check(0, 0); ret != protocol::Status::OK)
        {
            m_rx_malformed_packet++;
            return;
        }

        auto packet_counter = parser.getPacketCounter();
        if(packet_counter != m_last_rx_counter) {
            // @todo: handle packet loss
        }
        m_last_rx_counter = packet_counter;

        switch(m_state)
        {
            case State::Uninit:
                break;
            case State::WaitForHost:
                handshakeProcess(parser);
                break;
            case State::Connected:
                packetProcess(parser);
                break;
            default:
                m_state = State::Terminated;
                break;
        }
    }

    void handshakeProcess(protocol::eth::RxParser &parser)
    {
        if(auto ret = parser.getSpecialPacket(); ret.isOk()) {
            auto packet_code = ret.getValue();
            if(packet_code == 0x80) {
                auto ret_con = connect(m_sock_fd, (const struct sockaddr *)&m_client_addr, sizeof(m_client_addr));
                if(ret_con == 0) {
                    m_state = State::Connected;
                    // @todo: send handshake ack
                }
            }
        }
    }

    void packetProcess(protocol::eth::RxParser &parser)
    {
        while(!parser.isEnd()) {
            auto ret = parser.getNextPacket();
            if(!ret.isOk()) {
                m_rx_malformed_packet++;
                return;
            }

            auto& packet = ret.getValue();
            auto status = protocol::Interface::toDevice(packet);
            if(status != protocol::Status::OK) {
                m_rx_malformed_packet++;
            }
        }
    }

    void run()
    {
        uint8_t rx_buff[protocol::eth::Common::max_datagram_size+128];

        memset(&m_client_addr, 0, sizeof(m_client_addr)); 
        while(m_state != State::Terminated)
        {
            socklen_t len;
            size_t ret = recvfrom(m_sock_fd, (char *)rx_buff, sizeof(rx_buff), 
                                0, ( struct sockaddr *) &m_client_addr, &len); 

            if(ret <= 0) {
                continue;
            }

            protocol::DgBuff buff{rx_buff, ret};
            callback(buff);
        }
    }

    bool send(protocol::DgBuff& buff)
    {
        auto ret = sendto(m_sock_fd, buff.ptr(0), buff.size(), 0, 
                                   (const struct sockaddr *)&m_client_addr, sizeof(m_client_addr));
        if(ret != static_cast<ssize_t>(buff.size())) {
            // throw std::runtime_error("Failed to send data");
            m_tx_error_counter += 1;
            return false;
        }
        return true;
    }

private:
    template <class Rep, class Period>
    void setTimeout(std::chrono::duration<Rep,Period> duration)
    {
        using namespace std::chrono;

        auto sec = duration_cast<seconds>(duration);
        auto usec = duration_cast<microseconds>(duration - sec);

        struct timeval tv;
        tv.tv_sec = static_cast<time_t>(sec.count());
        tv.tv_usec = static_cast<suseconds_t>(usec.count());

        int ret = setsockopt(m_sock_fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv));
        if (ret < 0) {
            throw std::runtime_error("Failed to set timeout");
        }
    }

private:
    Config &m_config;

    int m_sock_fd{-1};
    struct sockaddr_in m_client_addr;

    uint32_t m_rx_malformed_packet{0};
    uint32_t m_last_rx_timestamp{0};
    uint16_t m_last_rx_counter{0};

    uint16_t m_tx_error_counter{0};

    protocol::StaticDgBuff<2048> m_tx_buff;
    protocol::eth::TxComposer m_tx_composer{m_tx_buff};

    std::atomic<State> m_state{State::Uninit};
    std::atomic<TxState> m_tx_state{TxState::Stopped};
};

} // namespace interface