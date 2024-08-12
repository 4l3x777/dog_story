#include <request_handler.h>
#include <boost/url.hpp>

namespace http_handler {

RequestHandler::RequestHandler(model::Game& game, const Args& program_args, Strand api_strand) : 
    file_response{program_args.www_root}, 
    api_response{*program_args.application, program_args.use_tick_api}, 
    api_strand_{api_strand} {}

std::string RequestHandler::UrlPathDecode(const std::string_view& path) {
    // url_path url-encoded decode
    boost::urls::pct_string_view s_url_path = path;
    std::string url_path;
    url_path.resize(s_url_path.decoded_size());
    s_url_path.decode({}, boost::urls::string_token::assign_to(url_path));
    return url_path;
}

void RequestHandler::GetResponseData(Response& res, std::shared_ptr<ResponseData> data) {
    if (std::holds_alternative<StringResponse>(res)) {
        (*data).status_code = (std::get<StringResponse>(res)).result_int(); 
        (*data).content_type = (std::get<StringResponse>(res))[http::field::content_type];
    }
    else {
        (*data).status_code = (std::get<FileResponse>(res)).result_int(); 
        (*data).content_type = (std::get<FileResponse>(res))[http::field::content_type];
    }
}

} // namespace http_handler
