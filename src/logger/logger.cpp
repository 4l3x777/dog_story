#include <logger.h>
#include <boost/log/trivial.hpp>     // для BOOST_LOG_TRIVIAL
#include <boost/log/core.hpp>        // для logging::core
#include <boost/log/expressions.hpp> // для выражения, задающего фильтр
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/date_time.hpp>
#include <boost/json.hpp>

namespace logger {
using namespace std::literals;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace expr = boost::log::expressions;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(data, "Data", boost::json::object)
BOOST_LOG_ATTRIBUTE_KEYWORD(msg, "Msg", std::string)

static void log_json(logging::record_view const& rec, logging::formatting_ostream& strm) {
    // Момент времени, приходится вручную конвертировать в строку.
    // Для получения истинного значения атрибута нужно добавить
    // разыменование. 
    auto ts = *rec[timestamp];
    boost::json::object log_data;
    log_data["timestamp"] = to_iso_extended_string(ts);
    log_data["data"] = *rec[data];
    log_data["message"] = *rec[msg];
    strm << boost::json::serialize(log_data) << std::endl;
}

void InitBoostLogFilter() {
    logging::core::get()->set_filter(
        logging::trivial::severity >= logging::trivial::info
    );
    logging::add_common_attributes();
    //log to file filter
    logging::add_file_log(
        keywords::file_name = "/var/log/sample_log_%N.log",
        keywords::format = &log_json,
        keywords::open_mode = std::ios_base::app | std::ios_base::out,
        // ротируем по достижению размера 10 мегабайт
        keywords::rotation_size = 10 * 1024 * 1024,
        // ротируем ежедневно в полдень
        keywords::time_based_rotation = sinks::file::rotation_at_time_point(12, 0, 0)
    );
    //log to console filter
    logging::add_console_log(
        std::cout,
        keywords::format = &log_json,
        keywords::auto_flush = true
    );
}

void Log::print(const boost::json::object& data_, const std::string& message_) {
    std::lock_guard<std::mutex> mutex{mutex_};
    BOOST_LOG_TRIVIAL(info) << logging::add_value(data, data_) << logging::add_value(msg, message_);
}

void LogMessage::server_start(const std::string& address, int port) {
    boost::json::object log_data;
    log_data["port"] = port;
    log_data["address"] = address.data();

    log_.print(log_data, "server started");
}

void LogMessage::server_stop(const boost::system::error_code& err) {
    boost::json::object log_data;
    log_data["code"] = err.value();
    if (err) {
        log_data["exception"] = err.what();
    }

    log_.print(log_data, "server exited");
}

void LogMessage::network_error(const sys::error_code& ec, where_type where)
{
    std::string where_;
    switch (where) {
    case where_type::read:
    {
        where_ = "read";
        break;
    }
    case where_type::write:
    {
        where_ = "write";
        break;
    }
    case where_type::accept:
    {
        where_ = "accept";
        break;
    }
    }

    boost::json::object log_data;
    log_data["code"] = ec.value();
    log_data["text"] = ec.message();
    log_data["where"] = where_;

    log_.print(log_data, "error");
}

void LogMessage::request_received(const std::string& address, const std::string& URI, const std::string& method)
{
    boost::json::object log_data;
    log_data["address"] = address;
    log_data["URI"] = URI;
    log_data["method"] = method;

    log_.print(log_data, "request received");
}

void LogMessage::response_sent(size_t response_time, unsigned status_code, const std::string& content_type)
{
    boost::json::object log_data;
    log_data["response_time"] = response_time;
    log_data["code"] = status_code;
    if (content_type.empty()) {
        log_data["content_type"] = "null";
    }
    else {
        log_data["content_type"] = content_type;
    }
    
    log_.print(log_data, "response sent");
}
}