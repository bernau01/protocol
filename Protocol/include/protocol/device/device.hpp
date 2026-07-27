#ifndef PROTOCOL_DEVICE_HPP
#define PROTOCOL_DEVICE_HPP

#include "protocol/utils/time.hpp"
#include "rx_parser.hpp"
#include "tx_composer.hpp"
#include "../data/dgbuff.hpp"
#include "../interface/port.hpp"
#include "../line/line.hpp"
#include "../packet/com_packet.hpp"
#include "../utils/list.hpp"
#include "../utils/lock.hpp"

#include <string_view>

namespace protocol
{

class Device
{

public:
    explicit Device(std::string_view name, std::string_view version, 
                    uint8_t sn, uint8_t addr, SpanList<Line*> lines) noexcept
        : m_name(name), m_version(version), m_sn(sn), m_addr(addr), m_lines(lines)
    {
        for(size_t i = 0; i < m_lines.getSize(); ++i) {
            if(m_lines[i]) {
                m_lines[i]->setDevice(this);
            }
        }
    }

    Device(const Device &other) = delete;
    Device &operator=(const Device &other) = delete;
    Device(Device &&other) = delete;
    Device &operator=(Device &&other) = delete;

    Status packetInput(const ComPacket& packet)
    {
        wakeUp();

        LockGuard lock_guard(m_still_receive);
        if (UNLIKELY(!lock_guard.tryLock())) {
            P_LOG_WARNING("Failed to acquire lock for packet input");
            return Status::Busy;
        }

        if(packet.sn != m_sn || packet.addr != m_addr) {
            P_LOG_INFO("Packet SN: {}, ADDR: {} does not match device SN: {}, ADDR: {}", 
                packet.sn, packet.addr, m_sn, m_addr);
            return Status::Refused;
        }
        else if(packet.addr == Common::broadcast_addr && packet.type == ComPacketType::Echo) {
            handleHandWave(packet);
            return Status::OK;
        }

        if(packet.isCommand()) {
            return processCommand(packet);
        }

        if(packet.port != m_host_port) {
            P_LOG_WARNING("The host haven't connected yet");
            return Status::Invalid;
        }

        RxParser parser{packet.data_buff, packet.num_of_nodes};

        auto status = Status::OK;

        while(!parser.isEnd()) {
            auto next_packet = parser.getNextPacket();
            if(!next_packet.isOk()) {
                status = next_packet.getStatus();
                break;
            }

            auto packet = next_packet.getValue();
            auto line_id = packet.getLineId();

            auto line = m_lines.get(line_id);
            if(!line.isOk()) {
                status = line.getStatus();
                break;
            }

            auto line_ptr = line.getValue();
            if(line_ptr == nullptr) {
                status = Status::NotFound;
                break;
            }

            auto ret = line_ptr->inputPacket(packet);
            if(ret != Status::OK) {
                // @todo: handle error
            }
        }

        if(status == Status::OK && parser.isRemaining()) {
            return Status::Malformed;
        }

        return status;
    }
    
    Status sendToHost(const Packet& packet)
    {
        auto status = m_tx_composer.place(packet);
        if(status != Status::OK) {
            return status;
        }

        triggerTransmitToHost();

        return status;
    }

    Status triggerTransmitToHost()
    {
        if(m_still_receive.isLocked()) {
            return Status::Busy;
        }

        auto sub_packet_count = m_tx_composer.getPacketCount();
        if(sub_packet_count == 0) {
            return Status::NothingToDo;
        }

        auto status = m_tx_composer.startTransmission();
        if(status != Status::OK) {
            return status;
        }

        ComPacket packet;
        packet.data_buff = m_tx_composer.getPayloadBuff();
        packet.num_of_nodes = sub_packet_count;

        transmitToHost(packet);

        m_tx_composer.endTransmissionAndReset();
        prepareTx();

        return Status::OK;
    }

    void update()
    {
        if(!isConnected()) {
            return;
        }

        auto current_time = utils::getCurrentTimestamp();
        if(current_time - m_timestamp >= conf::session::timeout_ms) {
            if(m_host_check_counter < conf::session::reconnect_attempts) {
                m_host_check_counter += 1;
                P_LOG_INFO("Checking host connection, attempt {}/{}", 
                    m_host_check_counter, conf::session::reconnect_attempts);
                handleCheckHost();
                m_timestamp = current_time - (conf::session::timeout_ms - conf::session::check_interval_ms);
            }
            else {
                P_LOG_WARNING("Device timeout, disconnecting host");
                disconnect();
            }
        }
        
    }

private:

    bool isConnected() const
    {
        return m_host_port != nullptr;
    }

    void wakeUp()
    {
        m_timestamp = utils::getCurrentTimestamp();
        m_host_check_counter = 0;
    }

    Status processCommand(const ComPacket& packet)
    {
        if(packet.data_buff.size() != 0) {
            return Status::Invalid;
        }

        switch(packet.type) {
        case ComPacketType::Echo:
            handleHandWave(packet);
            return Status::OK;

        case ComPacketType::Heartbeat:
            return Status::OK;

        case ComPacketType::ReqConnect:
            return handleConnect(packet);

        case ComPacketType::RespConnect:
            return handleIllegalCommand(packet);

        case ComPacketType::ReqDisconnect:
            return handleDisconnect(packet);

        case ComPacketType::RespDisconnect:
            return handleIllegalCommand(packet);

        default:
            return handleIllegalCommand(packet);
        }
    }

    Status handleConnect(const ComPacket& packet)
    {
        if(isConnected()) {
            if(packet.port != m_host_port) {
                handleBusy(packet);
                return Status::Busy;
            }
        }
        else {
            m_host_port = packet.port;
            m_port_info = m_host_port->getCurrentHostInfo();
        }
    
        ComPacket resp_packet;
        resp_packet.data_buff = DgBuff{};
        resp_packet.type = ComPacketType::RespConnect;

        P_LOG_INFO("Host connecting request accepted");

        return transmitToHost(resp_packet);
    }

    Status handleDisconnect(const ComPacket& packet)
    {
        if(!isConnected()) {
            handleInvalidCommand(packet);
            return Status::Invalid;
        }

        if(packet.port != m_host_port) {
            return Status::Invalid;
        }

        P_LOG_INFO("Host disconnected");

        return disconnect();
    }

    void handleHandWave(const ComPacket& packet)
    {
        auto data = packet.port->allocateTxDataBuff();

        size_t offset = 0;
        data.copyFrom("N:", 2, offset); 
        offset +=2;

        data.copyFrom(m_name.data(), m_name.size(), offset);
        offset += m_name.size();

        data.copyFrom(",V:", 3, offset);
        offset += 3;

        data.copyFrom(m_version.data(), m_version.size(), offset);
        offset += m_version.size();

        ComPacket resp_packet;
        resp_packet.data_buff = data.unsafeSub(0, offset);
        resp_packet.type = ComPacketType::Echo;
        resp_packet.sn = m_sn;
        resp_packet.addr = m_addr;
        resp_packet.counter = 0;

        P_LOG_INFO("Received hand wave from host, responding with device info: Name: {}, Version: {}", 
            m_name, m_version);

        auto host_info = packet.port->getCurrentHostInfo();
        packet.port->send(resp_packet, host_info);
    }

    Status handleBusy(const ComPacket& packet)
    {
        ComPacket resp_packet;

        resp_packet.data_buff = DgBuff{};
        resp_packet.sn = m_sn;
        resp_packet.addr = m_addr;
        resp_packet.counter = 0;
        resp_packet.type = ComPacketType::Busy;

        P_LOG_WARNING("Received command but device is busy, Type: {}", 
            getComPacketTypeName(packet.type));

        auto host_info = packet.port->getCurrentHostInfo();
        return packet.port->send(resp_packet, host_info);
    }

    Status handleInvalidCommand(const ComPacket& packet)
    {
        ComPacket resp_packet;

        resp_packet.data_buff = DgBuff{};
        resp_packet.sn = m_sn;
        resp_packet.addr = m_addr;
        resp_packet.counter = 0;
        resp_packet.type = ComPacketType::InvalidCmd;

        P_LOG_WARNING("Received invalid command, Type: {}", 
            getComPacketTypeName(packet.type));

        auto host_info = packet.port->getCurrentHostInfo();
        return packet.port->send(resp_packet, host_info);
    }

    Status handleIllegalCommand(const ComPacket& packet)
    {
        constexpr char response_code[] = 
            "\x53\x6F\x70\x6F\x20\x73\x65\x68\x20\x69\x6B\x69\x3F\x20\x4E"
            "\x67\x69\x72\x69\x6D\x20\x70\x61\x6B\x65\x74\x20\x67\x61\x6B"
            "\x20\x67\x65\x6E\x61\x68\x2C\x20\x6A\x2A\x6E\x63\x2A\x21";

        ComPacket resp_packet;

        auto data = packet.port->allocateTxDataBuff();
        if(data.copyFrom(response_code, sizeof(response_code), 0) == Status::OK) {
            resp_packet.data_buff = data.unsafeSub(0, sizeof(response_code));
        }
        else {
            resp_packet.data_buff = DgBuff{};
        }

        resp_packet.sn = m_sn;
        resp_packet.addr = m_addr;
        resp_packet.counter = 0;
        resp_packet.type = ComPacketType::IllegalCmd;

        P_LOG_WARNING("Received illegal command, Type: {}", 
            getComPacketTypeName(packet.type));

        auto host_info = packet.port->getCurrentHostInfo();
        return packet.port->send(resp_packet, host_info);
    }

    Status handleCheckHost()
    {
        if(!isConnected()) {
            return Status::NotFound;
        }

        ComPacket resp_packet;
        resp_packet.data_buff = DgBuff{};
        resp_packet.type = ComPacketType::CheckHost;

        auto status = transmitToHost(resp_packet);

        return status;
    }

    Status disconnect()
    {
        ComPacket resp_packet;
        resp_packet.data_buff = DgBuff{};
        resp_packet.type = ComPacketType::RespDisconnect;

        auto status = transmitToHost(resp_packet);

        m_host_port = nullptr;
        m_port_info = PortInfo{};

        m_tx_counter = 0;

        return status;
    }

    Status prepareTx()
    {
        if(!isConnected()) {
            return Status::Invalid;
        }

        return m_tx_composer.setBuff(m_host_port->allocateTxDataBuff());
    }

    Status transmitToHost(ComPacket& packet)
    {
        if(!isConnected()) {
            return Status::NotFound;
        }

        packet.sn = m_sn;
        packet.addr = m_addr;
        packet.counter = m_tx_counter++;

        return m_host_port->send(packet, m_port_info);
    }

private:
    std::string_view m_name;
    std::string_view m_version;

    const uint8_t m_sn = 0;
    const uint8_t m_addr = 0;

    SpanList<Line*> m_lines;

    Port* m_host_port = nullptr;
    PortInfo m_port_info{};

    TxComposer m_tx_composer;
    uint8_t m_tx_counter = 0;
    uint8_t m_host_check_counter = 0;
    uint32_t m_timestamp = 0;

    Lock m_still_receive;
};

}

#endif // PROTOCOL_DEVICE_HPP