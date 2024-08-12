#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <chrono>

namespace ticker {
namespace net = boost::asio;
namespace sys = boost::system;
using namespace std::chrono;
class Ticker : public std::enable_shared_from_this<Ticker> {
public:
    using Strand = net::strand<net::io_context::executor_type>;
    using Handler = std::function<void(uint64_t delta)>;
    using Clock = steady_clock;

    // Функция handler будет вызываться внутри strand с интервалом period
    Ticker(Strand strand, uint64_t period, Handler handler)
        : strand_{ strand }
        , period_{ std::chrono::milliseconds{period} }
        , handler_{ std::move(handler) } {}

    void Start();

private:
    void ScheduleTick();

    void OnTick(sys::error_code ec);

    Strand strand_;
    milliseconds period_;
    net::steady_timer timer_{strand_};
    Handler handler_;
    steady_clock::time_point last_tick_;
};
}