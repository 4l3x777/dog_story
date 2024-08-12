#include <network/rest_api/file.h>
#include <boost/url.hpp>
#include <network/mime_types.h>
#include <string_view>
#include <algorithm>

namespace http_handler {

Response File::MakeGetHeadResponse(const StringRequest& req) {
    const auto file_response = [&req, this]() {
        return MakeFileResponse(req.target(), req.version(), req.keep_alive());
    };

    return file_response();
}

Response File::MakePostResponse(const StringRequest& req) {
    return MakeUnknownMethodResponse(req);
}

Response File::MakeUnknownMethodResponse(const StringRequest& req) {
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

    return text_response(http::status::method_not_allowed, "Invalid method");
}

Response File::MakeFileResponse(
        std::string_view url_path, 
        unsigned http_version,
        bool keep_alive) {
    
    // url_path url-encoded decode
    boost::urls::pct_string_view s_url_path = url_path;
    std::string file_path;
    file_path.resize(s_url_path.decoded_size());
    s_url_path.decode({}, boost::urls::string_token::assign_to(file_path));
    std::replace(file_path.begin(), file_path.end(), '+', ' '); // replace all '+' to ' '


    // check path is directory
    if (*(file_path.rbegin()) == '/') {
        file_path += "index.html";
    }

    // set absolute path
    auto file_path_full = resource_root_path_ + file_path;

    // file bad response template
    const auto file_bad_response = [&](http::status status, std::string_view text) {
        return MakeStringResponse(status, text, http_version, keep_alive, "text/plain");
    }; 

    // check file_path_full contain root
    if (!PathContainRoot(file_path_full)) {
        return file_bad_response(http::status::bad_request, "Bad Request");
    }

    // try to open file if it exists
    http::file_body::value_type file;
    if (boost::system::error_code ec; file.open(file_path_full.c_str(), beast::file_mode::read, ec), ec) {
        return file_bad_response(http::status::not_found, "Failed to open file "s + file_path);
    }

    FileResponse response(http::status::ok, http_version);
    auto extention = std::filesystem::path(file_path_full).extension().string();
    response.set(http::field::content_type, mime_types::extension_to_mime_type(extention));
    response.body() = std::move(file);
    response.keep_alive(keep_alive);
    // Метод prepare_payload заполняет заголовки Content-Length и Transfer-Encoding
    // в зависимости от свойств тела сообщения
    response.prepare_payload();
    return std::move(response);
}

bool File::PathContainRoot(const std::string& path) {
    auto path_ = std::filesystem::weakly_canonical(path).string();
    if (path_.find(resource_root_path_) == 0) {
        return true;
    }
    return false;
}

};