#include <protocol/utils/log.hpp>

#include "logger.hpp"
#include "quill/backend/BackendOptions.h"

#include <filesystem>
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h>
#include <quill/LogMacros.h>

namespace itsr
{

static quill::Logger* s_logger;

// @todo: replace with relative project path or argument input
static std::filesystem::path s_log_path{"/home/zainir17/Documents/ITSR Protocol/protocol/log/"};

void logger_init()
{
    quill::BackendOptions backend_opts{};
    quill::Backend::start(backend_opts);

    auto sinks = std::vector<std::shared_ptr<quill::Sink>>{};

    sinks.emplace_back(quill::Frontend::create_or_get_sink<quill::ConsoleSink>("itsr_console_sink"));

    quill::FileSinkConfig file_sink_config{};
    file_sink_config.set_filename_append_option(quill::FilenameAppendOption::StartDateTime);
    file_sink_config.set_open_mode('w');

    auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(s_log_path, file_sink_config);
    sinks.push_back(file_sink);

    s_logger = quill::Frontend::create_or_get_logger("itsr_logger", sinks);
    s_logger->set_log_level(quill::LogLevel::TraceL3);

    LOG_INFO("The log will saved in : {}", static_cast<quill::FileSink*>(file_sink.get())->get_filename().c_str());

}

void logDispatchImpl(protocol::log::LogLevel level, const std::string& message)
{
    if (s_logger) {
        switch (level) {
            case protocol::log::LogLevel::Debug:
                LOG_DEBUG("{}", message);
                break;
            case protocol::log::LogLevel::Info:
                LOG_INFO("{}", message);
                break;
            case protocol::log::LogLevel::Warning:
                LOG_WARNING("{}", message);
                break;
            case protocol::log::LogLevel::Error:
                LOG_ERROR("{}", message);
                break;
        }
    }
}

auto logger() -> quill::Logger* const
{
    return s_logger;
}

} // namespace itsr

namespace protocol::log
{
    LogFunc g_log_func = itsr::logDispatchImpl;
} // namespace protocol::log