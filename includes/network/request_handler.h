#pragma once
#include <network/http_server.h>
#include <model.h>
#include <syncstream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <variant>
#include <filesystem>
#include <boost/json.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <logger/logger.h>
#include <network/rest_api/file.h>
#include <network/rest_api/api.h>
#include <chrono>
#include <string>
#include <memory>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;

using namespace std::literals;

struct Args {
    std::vector<std::string> source;
    std::string config_file;
    std::string save_file;
    std::string www_root;
    uint64_t tick_time;
    bool use_tick_api {false};
    bool randomize_spawn_points {false};
    std::chrono::milliseconds save_state_period;
    std::shared_ptr<application::Application> application;
    bool save_state {false};
};

struct ResponseData {
    std::string content_type;
    int status_code;
    // add other fileds it need
};

class RequestHandler {
public:
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;

    RequestHandler(model::Game& game, const Args& program_args, Strand api_strand);
    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, std::shared_ptr<ResponseData> response_data) {
        auto version = req.version();
        auto keep_alive = req.keep_alive();

        // url_path url-encoded decode
        auto url_path = UrlPathDecode(req.target());

        try {
            /*req относится к API?*/
            if (url_path.find("/api") == 0) {
                auto handle = [
                    this, 
                    send,
                    &req,
                    response_data,
                    version, 
                    keep_alive
                ] {
                    try {
                        // Этот assert не выстрелит, так как лямбда-функция будет выполняться внутри strand
                        assert(api_strand_.running_in_this_thread());
                        auto handled_req = api_response.HandleRequest(std::forward<decltype(req)>(req));
                        // get response data
                        GetResponseData(handled_req, response_data);
                        return SendRequest(handled_req, send);
                    }
                    catch (...) {
                        //send(self->ReportServerError(version, keep_alive));
                    }
                };
                return boost::asio::dispatch(api_strand_, handle);
            }
            else {
                // Возвращаем результат обработки запроса к файлу
                auto handled_req = file_response.HandleRequest(std::forward<decltype(req)>(req));
                // get response data
                GetResponseData(handled_req, response_data);
                return SendRequest(handled_req, send);
            }
        }
        catch (...) {
            //auto text = app::JsonMessage("ServerError"sv,
                //"request processing error"sv);
            //send(Base::MakeStringResponse(http::status::bad_request, text, version, keep_alive));
            //send(ReportServerError(version, keep_alive));
        }
    }

private:
    Strand api_strand_;
    File file_response;
    Api api_response;
 
    std::string UrlPathDecode(const std::string_view& path);
 
    void GetResponseData(Response& res, std::shared_ptr<ResponseData> data);

    template<typename Send>
    void SendRequest(Response& res, Send&& send) {
        if (std::holds_alternative<StringResponse>(res)) {
            send(std::get<StringResponse>(res));
        }
        else {
            send(std::get<FileResponse>(res));
        }
    }

    //StringResponse ReportServerError(unsigned version, bool keep_alive);

};

// decorator pattern for RequestHandler
template<class SomeRequestHandler>
class LoggingRequestHandler {

    template<typename Stream>
    void LogRequest(const StringRequest& req, Stream&& stream) {
        LOG_MSG().request_received(
            stream.socket().remote_endpoint().address().to_string(), 
            req.target(), 
            http::to_string(req.method())
        );
    }

    void LogResponse(ResponseData data) {
        LOG_MSG().response_sent(
            std::chrono::round<std::chrono::milliseconds>(res_end_time_ - res_start_time_).count(), 
            data.status_code, 
            data.content_type
        );
    }
public:
    LoggingRequestHandler(SomeRequestHandler& handler) : decorated_(handler) {}

    template <typename Body, typename Allocator, typename Send, typename Stream>
    void operator ()(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send, Stream&& stream) {
        // logging request
        LogRequest(req, stream);

        // prepare response
        res_start_time_ = std::chrono::steady_clock::now();

        auto res_data = std::make_shared<ResponseData>();
        decorated_(std::move(req), std::move(send), res_data);

        res_end_time_ = std::chrono::steady_clock::now();

        // logging response data
        LogResponse(*res_data);
    }

private:
    SomeRequestHandler& decorated_;
    std::chrono::steady_clock::time_point res_start_time_;
    std::chrono::steady_clock::time_point res_end_time_;
};

}  // namespace http_handler
