#include <network/rest_api/response_base.h>
#include <boost/beast/http.hpp>
#include <boost/json/src.hpp>

namespace http_handler {

// Создаёт StringResponse с заданными параметрами
StringResponse ResponseBase::MakeStringResponse(
    http::status status, 
    std::string_view body, 
    unsigned http_version,
    bool keep_alive,
    std::string_view content_type,
    bool no_cache,
    std::string_view allow_methods) 
{
    StringResponse response(status, http_version);
    if (no_cache) {
        response.set(http::field::cache_control, "no-cache"sv);
    }
    response.set(http::field::allow, allow_methods);
    response.set(http::field::content_type, content_type);
    response.body() = body;
    response.content_length(body.size());
    response.keep_alive(keep_alive);
    return response;
}

std::string ResponseBase::SerializeMessageCode(const std::string_view& code, const std::string_view& message) {
    boost::json::object response;
    response["code"] = code.data();
    response["message"] = message.data();
    return boost::json::serialize(response);
}

Response ResponseBase::HandleRequest(const StringRequest& req) {
    // Format response
    const auto text_response = [&req, this](http::status status, std::string_view text) {
        return MakeStringResponse(
            status, 
            text, 
            req.version(), 
            req.keep_alive(),
            ContentType::APPLICATION_JSON,
            true,
            "GET, HEAD"sv);
    };  
    try {
        switch (req.method()) {
            case http::verb::get:
            {
                return MakeGetHeadResponse(req);
            }
            case http::verb::head:
            {
                return MakeGetHeadResponse(req);
            }
            case http::verb::post:
            {
                return MakePostResponse(req);
            }
            default:
            {
                return MakeUnknownMethodResponse(req);
                //return text_response(http::status::method_not_allowed, "Invalid method"sv);
            }
        }
    }
    catch (const std::exception& ex) {
        auto response_status = http::status::bad_request;
        auto response = SerializeMessageCode("badRequest", "Bad request"sv);
        return text_response(response_status, response);
    }
}

}