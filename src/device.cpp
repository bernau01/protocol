#include "device.hpp"
#include "protocol/node/time_node.hpp"

#include "interface/udp_linux.hpp"

#include <numeric>
#include <protocol/line/internal.hpp>
#include <protocol/node/node.hpp>
#include <protocol/line/line.hpp>
#include <protocol/interface/interface.hpp>
#include <protocol/device/device.hpp>

#include <tuple>

#include <cstring>

namespace device
{

interface::eth::Config udp_config{
    .ip_addr = "192.168.2.0",
    .port = 1234
};

static inline constexpr uint16_t DEVICE_ID = 0x01;

volatile uint16_t address = 0x01;

static auto time_node_1 = TimeNode{1};
auto node_1 = protocol::Node<32>{2};

auto udp_line_1 = interface::eth::UdpLinux{udp_config};

auto internal_line = protocol::InternalLine{
    protocol::NodeSet<&time_node_1>(),
    protocol::NodeSet<&node_1>()
};

auto dev = protocol::Device{
    std::make_tuple(
        protocol::InterfaceSet<&udp_line_1>()
    ),
    std::make_tuple(
        protocol::LineSet<&internal_line>()
    )
};

void init() 
{
    udp_line_1.init();
}

void update()
{
    
}

uint16_t getDeviceId() 
{
    return DEVICE_ID;
}

uint16_t getAddress()
{
    return address;
}

}