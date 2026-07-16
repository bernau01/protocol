#ifndef PROTOCOL_DEVICE_FUNC_SET_HPP
#define PROTOCOL_DEVICE_FUNC_SET_HPP

#include "../packet/packet.hpp"
#include "../status.hpp"
#include "../utils/func_set.hpp"

namespace protocol
{

using ObjFuncPtr = FuncPtr<Status, Packet&>;
using RouteFuncSet = FuncSet<void, Status, Packet&>;

}

#endif // PROTOCOL_DEVICE_FUNC_SET_HPP