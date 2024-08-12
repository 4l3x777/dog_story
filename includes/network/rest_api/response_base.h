#pragma once
#include <sdk.h>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <filesystem>
#include <iostream>
#include <variant>
#include <string_view>

namespace http_handler {
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http; 
namespace fs = std::filesystem;
namespace sys = boost::system;
using namespace std::literals;

// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
// Response, where body is file
using FileResponse = http::response<http::file_body>;
// Response variant, body is string or file
using Response = std::variant<StringResponse, FileResponse>;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
    // При необходимости внутрь ContentType можно добавить и другие типы контента
};

class ResponseBase {
public:
    Response HandleRequest(const StringRequest& req);
    
    StringResponse MakeStringResponse(
        http::status status, 
        std::string_view requestTarget, 
        unsigned http_version,
        bool keep_alive, 
        std::string_view content_type = ContentType::APPLICATION_JSON,
        bool no_cache = true,
        std::string_view allow_methods = "GET, HEAD, POST"sv);

    std::string SerializeMessageCode(const std::string_view& code, const std::string_view& message);

protected:
    virtual Response MakeGetHeadResponse(const StringRequest& req) = 0;
    virtual Response MakePostResponse(const StringRequest& req) = 0;
    virtual Response MakeUnknownMethodResponse(const StringRequest& req) = 0;
};

};