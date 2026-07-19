#pragma once

#include <logger.hpp>
#include <fmt/core.h>

class DummyCanDriver
{
public:
    static constexpr uint32_t efe_flag = 0x80000000;
    static constexpr uint32_t rtr_flag = 0x40000000;
    
public:
    DummyCanDriver() = default;

    void send(uint32_t can_id, uint32_t flag, const uint8_t* data, size_t len)
    {
        LOG_INFO("Transmitting data over CAN: ");
        std::string hex_str;
        for (size_t i = 0; i < len; ++i) {
            hex_str += fmt::format("{:02X} ", data[i]);
        }
        LOG_INFO("{:08X} | {:08X} | {}", can_id, flag, hex_str);
    }
};