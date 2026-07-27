#ifndef PROTOCOL_CONF_HPP
#define PROTOCOL_CONF_HPP

#if __has_include(<protocol_config.hpp>)
    #include <protocol_config.hpp>
#endif

#ifndef PROTOCOL_CONFIG
#define PROTOCOL_CONFIG
#define PROTOCOL_CONFIG_DEFAULT

#define PROTOCOL_SESSION_TIMEOUT_MS 5000
#define PROTOCOL_SESSION_CHECK_INTERVAL_MS 500
#define PROTOCOL_SESSION_RECONNECT_ATTEMPTS 3

#endif // PROTOCOL_CONFIG

#include <cstdint>

namespace protocol::conf
{

namespace session
{
    constexpr uint32_t timeout_ms = PROTOCOL_SESSION_TIMEOUT_MS;
    constexpr uint32_t check_interval_ms = PROTOCOL_SESSION_CHECK_INTERVAL_MS;
    constexpr uint32_t reconnect_attempts = PROTOCOL_SESSION_RECONNECT_ATTEMPTS;
} // namespace session

} // namespace protocol::conf

#endif // PROTOCOL_CONF_HPP