#pragma once
#include <network/rest_api/response_base.h>
#include <string>
#include <filesystem>

namespace http_handler {
	
class File : public ResponseBase {

	std::string resource_root_path_;

public:
	File(const std::string& resource_root_path) {
        // check & set resource_root_path_
        auto root_path = std::filesystem::weakly_canonical(std::filesystem::path(resource_root_path));
        if (!std::filesystem::is_directory(root_path)) {
            throw std::invalid_argument("Resource root path "s + root_path.generic_string() + " not exists!");
        }
        resource_root_path_ = root_path.generic_string();
    }

private: 
    virtual Response MakeGetHeadResponse(const StringRequest& req) override;
	virtual Response MakePostResponse(const StringRequest& req) override;
    virtual Response MakeUnknownMethodResponse(const StringRequest& req) override;

    Response MakeFileResponse(
        std::string_view url_path, 
        unsigned http_version,
        bool keep_alive);

    bool PathContainRoot(const std::string& path);

};

} // namespace http_handler