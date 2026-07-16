#ifndef PROTOCOL_UTILS_FUNC_SET_HPP
#define PROTOCOL_UTILS_FUNC_SET_HPP

#include "common.hpp"
#include "status.hpp"

#include <utility>

namespace protocol
{

template <typename TArg>
using VisitFunc = Status (*)(void *, TArg &);

template <typename TArg>
using FuncSet = std::pair<void *, VisitFunc<TArg>>;

}

#endif // PROTOCOL_UTILS_FUNC_SET_HPP