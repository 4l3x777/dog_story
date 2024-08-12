#pragma once 
#include <ostream>
#include <iostream>
#include <mutex>
#include <boost/json.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/system.hpp>

#define LOG() logger::Log::GetInstance()
#define LOG_MSG() logger::LogMessage::GetInstance()

namespace logger {

namespace sys = boost::system;
namespace net = boost::asio;
namespace json = boost::json;
    
void InitBoostLogFilter();

class Log {
    Log() = default;
    Log(const Log&) = delete;
public:
    static Log& GetInstance() {
        static Log obj;
        return obj;
    }

    void print(const boost::json::object& data, const std::string& message);
private:
    std::ostream& os_ {std::cout};
    std::mutex mutex_;
};

class LogMessage {
private:
    Log& log_; 
    LogMessage() : log_(Log::GetInstance()) {}
public:

    enum where_type {
        read,
        write,
        accept,
        close
    };

    static LogMessage& GetInstance() {
        static LogMessage obj;
        return obj;
    }

    void server_start(const std::string& address, int port);
    void server_stop(const sys::error_code& ec);
    void network_error(const sys::error_code& ec, where_type where);
    void request_received(const std::string& IP, const std::string& URI, const std::string& method);
    void response_sent(size_t response_time, unsigned status_code, const std::string& content_type);
};



} // namespace logger