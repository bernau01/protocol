#pragma once

#define QUILL_DISABLE_NON_PREFIXED_MACROS
#include <quill/Logger.h>
#include <quill/LogMacros.h>

namespace itsr
{
void logger_init();

quill::Logger* const logger();

} // namespace itsr

#define LOG_DEBUG(fmt, ...)     QUILL_LOG_DEBUG(itsr::logger(), fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)      QUILL_LOG_INFO(itsr::logger(), fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...)   QUILL_LOG_WARNING(itsr::logger(), fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)     QUILL_LOG_ERROR(itsr::logger(), fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...)  QUILL_LOG_CRITICAL(itsr::logger(), fmt, ##__VA_ARGS__)