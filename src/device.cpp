#include "device.hpp"
#include "protocol/node/time_node.hpp"

#include "protocol/port/udp.hpp"
#include "driver/dummy_can.hpp"
#include "driver/dummy_uart.hpp"

#include <protocol/line/internal.hpp>
#include <protocol/node/node.hpp>
#include <protocol/line/line.hpp>
#include <protocol/interface/interface.hpp>
#include <protocol/device/device.hpp>

#include <protocol/can_line/can_line.hpp>
#include <protocol/uart_line/uart_line.hpp>

#include <cstring>

namespace device
{

auto udp_config = UdpConfig{
    .ip_addr = "0.0.0.0",
    .port = 12345
};

uint8_t serial_number = 0x01;
uint8_t address = 0x01;

auto time_node_1 = TimeNode{1};
auto node_1 = protocol::Node<32>{2};

auto internal_line = protocol::InternalLine{
    protocol::NodeSet<&time_node_1>(),
    protocol::NodeSet<&node_1>()
};

auto can_driver = DummyCanDriver{};
auto can_line_1 = protocol::CanLine<DummyCanDriver>{can_driver};

auto uart_driver = DummyUartDriver{};
auto uart_line_1 = protocol::UartLine<DummyUartDriver>{uart_driver};

auto line_list = protocol::StaticList<protocol::Line*, 4>{
    &internal_line,
    &can_line_1,
    nullptr,
    &uart_line_1,
};

auto dev = protocol::Device{
    "Main Board",
    "1.0",
    serial_number,
    address,
    line_list
};

auto port_udp_1 = UdpPort{udp_config, &dev};

void init() 
{
    port_udp_1.start();
}

void update()
{
    port_udp_1.join();
}

}