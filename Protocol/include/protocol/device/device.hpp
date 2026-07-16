#ifndef PROTOCOL_DEVICE_HPP
#define PROTOCOL_DEVICE_HPP

#include "dev_func_set.hpp"
#include "dev_child.hpp"

#include <tuple>
#include <array>
#include <type_traits>
#include <limits>

namespace protocol
{

template <typename TupleIfType, typename TupleLineType>
class Device 
{

public:
    explicit constexpr Device(const TupleIfType& interface, const TupleLineType& line) 
        : interfaces_sets(getTupleToArray(interface)), 
          line_sets(getTupleToArray(line))
    {
        auto func1 = RouteFuncSet(this, routeToLine);
        auto func2 = RouteFuncSet(this, routeToInterface);
        std::apply([&func1](const auto& ...args) {
            (setDevice(args, func1), ...);
        }, interface);
        std::apply([&func2](const auto& ...args) {
            (setDevice(args, func2), ...);
        }, line);
    }

    __attribute__((always_inline))
    Status fromInterface(Packet& packet) 
    {
        if(packet.getLineId() >= line_sets.size()) {
            return Status::NotFound;
        }
        return line_sets[packet.getLineId()](packet);
    }

    __attribute__((always_inline))
    Status fromLine(Packet& packet) 
    {
        if(packet.getInterfaceId() == std::numeric_limits<uint8_t>::max()) {
            auto status = Status::NotFound;
            for(auto& line_func : line_sets) {
                auto ret = line_func(packet);
                if(status != Status::OK) {
                    status = ret;
                }
            }
            if(status != Status::OK) {
                status = Status::NotFound;
            }
            return status;
        }
        else if(packet.getInterfaceId() >= interfaces_sets.size()) {
            return Status::NotFound;
        }
        return interfaces_sets[packet.getInterfaceId()](packet);
    }

private:

    __attribute__((always_inline))
    static inline constexpr Status routeToLine(void* instance, Packet& packet) 
    {
        return static_cast<Device*>(instance)->fromInterface(packet);
    }

    __attribute__((always_inline))
    static inline constexpr Status routeToInterface(void* instance, Packet& packet) 
    {
        return static_cast<Device*>(instance)->fromLine(packet);
    }

    template <typename ...TArgs>
    __attribute__((always_inline))
    static inline constexpr auto getTupleToArray(const std::tuple<TArgs...>& tuple) 
    {
        // static_assert((std::is_base_of_v<DevChild, typename TArgs::InstanceType> && ...), "All instances must be derived from DevChild");
        return std::apply([](auto... args) {
            return std::array<ObjFuncPtr, sizeof...(args)>{getCallFunction(args)...};
        }, tuple);
    }

    template <typename T>
    __attribute__((always_inline))
    static inline constexpr auto getCallFunction(const T& func_set) 
    {
        if constexpr (std::is_base_of_v<StaticFuncSetTag, T>) {
            static_assert(std::is_base_of_v<DevChild, typename T::InstanceType>, "All instances must be derived from DevChild");
            return func_set.callFunction;
        } else {
            return nullFunc;
        }
    }

    template <typename T>
    __attribute__((always_inline))
    static inline constexpr auto setDevice(const T& func_set, const RouteFuncSet& func) 
    {
        if constexpr (std::is_base_of_v<StaticFuncSetTag, T>) {
            static_assert(std::is_base_of_v<DevChild, typename T::InstanceType>, "All instances must be derived from DevChild");
            func_set.instance->setDevice(func);
        } 
    }

    static inline constexpr Status nullFunc(Packet& packet) 
    {
        (void)packet; // To avoid unused parameter warning
        return Status::NotFound;
    }

    const std::array<ObjFuncPtr, std::tuple_size_v<TupleIfType>> interfaces_sets;
    const std::array<ObjFuncPtr, std::tuple_size_v<TupleLineType>> line_sets;
};


}

#endif // PROTOCOL_DEVICE_HPP