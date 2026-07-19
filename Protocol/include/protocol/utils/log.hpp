#ifndef PROTOCOL_LOG_HPP
#define PROTOCOL_LOG_HPP

#ifdef PROTOCOL_LOGGING_ENABLED

#include <fmt/core.h>

namespace protocol::log
{

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error
};

using LogFunc = void(*)(LogLevel level, const std::string& message);
extern LogFunc g_log_func;

inline void logDispatch(LogLevel level, const std::string& message)
{
    if (g_log_func) {
        g_log_func(level, message);
    }
}

} // namespace protocol::log

#define P_LOG_DEBUG(message, ...)   protocol::log::logDispatch(protocol::log::LogLevel::Debug, fmt::format(message, ##__VA_ARGS__))
#define P_LOG_INFO(message, ...)    protocol::log::logDispatch(protocol::log::LogLevel::Info, fmt::format(message, ##__VA_ARGS__))
#define P_LOG_WARNING(message, ...) protocol::log::logDispatch(protocol::log::LogLevel::Warning, fmt::format(message, ##__VA_ARGS__))
#define P_LOG_ERROR(message, ...)   protocol::log::logDispatch(protocol::log::LogLevel::Error, fmt::format(message, ##__VA_ARGS__))

#else

#define P_LOG_DEBUG(message, ...)
#define P_LOG_INFO(message, ...)
#define P_LOG_WARNING(message, ...)
#define P_LOG_ERROR(message, ...)   

#endif // PROTOCOL_LOGGING_ENABLED

#endif // PROTOCOL_LOG_HPP