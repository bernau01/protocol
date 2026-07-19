#pragma once

#include <logger.hpp>
#include <fmt/core.h>

class DummyUartDriver
{
public:
    DummyUartDriver() = default;

    void transmit(const uint8_t* data, size_t len)
    {
        LOG_INFO("Transmitting data over UART: ");
        std::string hex_str;
        for (size_t i = 0; i < len; ++i) {
            hex_str += fmt::format("{:02X} ", data[i]);
        }
        LOG_INFO("{}", hex_str);
    }
};