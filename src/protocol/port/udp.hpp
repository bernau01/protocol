#pragma once

#include <protocol/device/device.hpp>
#include <protocol/interface/port.hpp>
#include <protocol/data/dgbuff.hpp>
#include <protocol/ethernet/eth_packet.hpp>
#include <protocol/packet/com_packet.hpp>
#include <protocol/line/line.hpp>

#include <bits/stdc++.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h> 

#include <stdexcept>
#include <thread>
#include <mutex>

#include "../../logger.hpp"

struct UdpConfig 
{
    char ip_addr[16];
    uint16_t port;
};

class UdpPort : public protocol::Port
{
public:
    explicit UdpPort(UdpConfig& config, protocol::Device* device) : protocol::Port(device), m_config(config) {}

    ~UdpPort()
    {
        m_is_terminated = true;
        close(m_sock_fd);
        join();
    }

    bool start()
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

        m_rx_thread = std::thread(&UdpPort::receiveThread, this);
        m_tx_monitor_thread = std::thread(&UdpPort::updateThread, this);

        return true;
    }

    protocol::Status send(protocol::ComPacket& packet, const protocol::PortInfo& port_info) override
    {
        if(packet.data_buff.ptr(0) != m_tx_buff.ptr(protocol::Common::datagram_offset)) {
            auto ret = m_tx_buff.copyFrom(
                packet.data_buff, 
                packet.data_buff.size(), 
                protocol::Common::datagram_offset
            );

            if(ret != protocol::Status::OK) {
                return ret;
            }
        }

        protocol::eth::PacketField packet_field{m_tx_buff};
        packet_field.writePreamble();
        packet_field.setDevSn(packet.sn);
        packet_field.setDevAddr(packet.addr);
        packet_field.setCounter(packet.counter);
        packet_field.setNumOfNodes(packet.num_of_nodes);

        struct sockaddr_in *client_addr = (struct sockaddr_in *)port_info.host_info;

        auto raw_buff = m_tx_buff.unsafeSub(0, protocol::Common::comm_header_size + packet.data_buff.size());
        sendRaw(raw_buff, client_addr);

        return protocol::Status::OK;
    }

    protocol::PortInfo getCurrentHostInfo() override
    {
        protocol::PortInfo info;
        info.max_node = 64;
        setClientAddr(m_client_addr, info);
        return info;
    }

    protocol::DgBuff allocateTxDataBuff() override
    {
        return m_tx_buff.unsafeSub(protocol::Common::datagram_offset);
    }

    void receiveThread()
    {
        LOG_INFO("Receive thread is started");
        uint8_t rx_buff[1500];

        memset(&m_client_addr, 0, sizeof(m_client_addr)); 
        while(!m_is_terminated)
        {
            socklen_t len = sizeof(m_client_addr);
            auto ret = recvfrom(m_sock_fd, (char *)rx_buff, sizeof(rx_buff), 
                                0, ( struct sockaddr *) &m_client_addr, &len); 

            if(ret <= 0) {
                continue;
            }

            protocol::DgBuff buff{rx_buff, static_cast<size_t>(ret)};

            std::lock_guard<std::mutex> lock(m_mutex);
            callback(buff);
        }
    }

    void updateThread()
    {
        LOG_INFO("Transmit checker thread is started");
        while(!m_is_terminated) {
            do {
                std::lock_guard<std::mutex> lock(m_mutex);
                auto ret = getDevice()->triggerTransmitToHost();
                if(ret == protocol::Status::OK) {
                    LOG_INFO("Transmit to host is triggered");
                }
            } while(false);

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    void join() 
    {
        if(m_rx_thread.joinable()) {
            m_rx_thread.join();
        }
        if(m_tx_monitor_thread.joinable()) {
            m_tx_monitor_thread.join();
        }
    }

private:
    void callback(protocol::DgBuff& buff)
    {
        protocol::eth::PacketField packet_field{buff};
        if(!packet_field.isPreambleValid()) {
            return;
        }

        protocol::ComPacket packet;
        packet.sn = packet_field.getDevSn();
        packet.addr = packet_field.getDevAddr();
        packet.counter = packet_field.getCounter();
        packet.num_of_nodes = packet_field.getNumOfNodes();
        packet.data_buff = buff.unsafeSub(protocol::Common::datagram_offset);
        packet.port = this;

        LOG_INFO("Received packet\nSN: {}, Addr: {}, Counter: {}, Num of Nodes: {}", 
            packet.sn, packet.addr, packet.counter, packet.num_of_nodes);
        auto ret = getDevice()->packetInput(packet);

        if(ret != protocol::Status::OK) {
            LOG_ERROR("Failed to process packet: Status::{}", getStatusName(ret));
        }
    }

    bool sendRaw(protocol::DgBuff& buff, struct sockaddr_in *m_addr)
    {
        auto ret = sendto(m_sock_fd, buff.ptr(0), buff.size(), 0, 
                                   (const struct sockaddr *)m_addr, sizeof(struct sockaddr_in));
        if(ret != static_cast<ssize_t>(buff.size())) {
            // throw std::runtime_error("Failed to send data");
            m_tx_error_counter += 1;
            return false;
        }

        LOG_INFO("Sent packet {} bytes", ret);

        return true;
    }

    void getClientAddr(const protocol::PortInfo& src, struct sockaddr_in& dest) const
    {
        memcpy(&dest, src.host_info, sizeof(m_client_addr));
    }

    void setClientAddr(const struct sockaddr_in& src, protocol::PortInfo& dest) const
    {
        memcpy(dest.host_info, &src, sizeof(m_client_addr));
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
    UdpConfig &m_config;

    int m_sock_fd{-1};
    struct sockaddr_in m_client_addr;

    protocol::StaticDgBuff<2048> m_tx_buff;
    uint32_t m_tx_error_counter = 0;

    std::thread m_rx_thread;
    std::thread m_tx_monitor_thread;
    std::mutex m_mutex;
    std::atomic_bool m_is_terminated{false};
};