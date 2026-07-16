#pragma once

#include <protocol/node/node.hpp>
#include <chrono>

class TimeNode : public protocol::Node<8>
{
public:
    TimeNode(size_t id) : protocol::Node<8>(id) {}

    void update()
    {
        using namespace std::chrono;
        auto now = system_clock::now();
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()).count();
        setSegData(0, static_cast<uint32_t>(ms));

        auto sec = duration_cast<seconds>(now.time_since_epoch()).count();
        setSegData(1, static_cast<uint32_t>(sec));
    }
};