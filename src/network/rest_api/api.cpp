#include <network/rest_api/api.h>
#include <network/http_server.h>
#include <cassert>
#include <boost/url.hpp>

namespace http_handler {

API_TYPE Api::RestApiHandlerType(const std::string_view& request_target) {    
    if (request_target.find("/api/v1/maps"s) == 0) {
        return API_TYPE::MAPS_API;
    }
    else if (request_target.find("/api/v1/game/join"s) == 0) {
        return API_TYPE::GAME_JOIN;
    }
    else if (request_target.find("/api/v1/game/players"s) == 0) {
        return API_TYPE::GAME_PLAYERS;
    }
    else if (request_target.find("/api/v1/game/state"s) == 0) {
        return API_TYPE::GAME_STATE;
    }
    else if (request_target.find("/api/v1/game/player/action"s) == 0) {
        return API_TYPE::GAME_PLAYER_ACTION;
    }
    else if (request_target.find("/api/v1/game/tick"s) == 0) {
        if (use_tick_api_) {
            return API_TYPE::GAME_TICK;
        }
        else {
            assert(!"Bad API type");
            return API_TYPE::UNKNOWN;
        }
    }
    else if (request_target.find("/api/v1/game/records"s) == 0) {
        return API_TYPE::GAME_RECORDS;
    }
    else {
        assert(!"Bad API type");
        return API_TYPE::UNKNOWN;
    }
}

Response Api::MakeGetHeadResponse(const StringRequest& req) {
    
    const auto text_response = [&](http::status status, std::string text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    const auto invalid_method_post = [&](){
        auto response_status = http::status::method_not_allowed;
        auto response = SerializeMessageCode("invalidMethod", "Only POST method is expected");
        return MakeStringResponse(
            response_status, 
            response,
            req.version(), 
            req.keep_alive(),
            ContentType::APPLICATION_JSON,
            true,
            "POST"sv);
    };

    auto request_target = req.target();
    auto api_handler_type = RestApiHandlerType(request_target);

    switch(api_handler_type)
    {
        case API_TYPE::MAPS_API:
        {
            http::status response_status;
            auto response = app_.GetMapJson(request_target, response_status);
            return text_response(response_status, response);

        }
        case API_TYPE::GAME_JOIN:
        {
            return invalid_method_post();
        }
        case API_TYPE::GAME_PLAYERS:
        {
            return GetPlayers(req);
        }
        case API_TYPE::GAME_STATE:
        {
            return GetState(req);
        }
        case API_TYPE::GAME_PLAYER_ACTION:
        {
            return invalid_method_post();
        }
        case API_TYPE::GAME_TICK:
        {
            return invalid_method_post();
        }
        case API_TYPE::GAME_RECORDS:
        {
            try {
                int start = 0, max_items = 100;
                auto endpoint = boost::urls::url(req.target());
                for (auto [k, v, h] : endpoint.params()) {
                    if (k == "start") {
                        start = std::stoi(v);
                    }
                    else if (k == "maxItems") {
                        max_items = std::stoi(v);
                    }
                }

                if (start < 0 ||
                    max_items > 100 ||
                    max_items <= 0
                ) {
                    throw std::logic_error("GAME_RECORDS: Invalid input params");
                }
                else {
                    // get records
                    application::APPLICATION_ERROR app_error;
                    auto response = app_.GetRecords(start, max_items, app_error);

                    switch (app_error) {
                        case application::APPLICATION_ERROR::INVALID_ARGUMENT:
                        {
                            return text_response(http::status::bad_request, response);
                        }
                        default:
                        {
                            return text_response(http::status::ok, response);
                        }
                    }
                }
            }
            catch (const std::exception& ex) {
                auto response_status = http::status::bad_request;
                auto response = SerializeMessageCode("badRequest", "Bad request");
                return text_response(response_status, response);
            }
        }
        default:
        {
            auto response_status = http::status::bad_request;
            auto response = SerializeMessageCode("badRequest", "Bad request");
            return text_response(response_status, response);
        }
    }
}

Response Api::MakePostResponse(const StringRequest& req) {
    const auto text_response = [&](http::status status, std::string text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    const auto invalid_method_get_head = [&](){
        auto response_status = http::status::method_not_allowed;
        auto response = SerializeMessageCode("invalidMethod", "Invalid method");
        return MakeStringResponse(
            response_status, 
            response,
            req.version(), 
            req.keep_alive(),
            ContentType::APPLICATION_JSON,
            true,
            "GET, HEAD"sv);
    };

    auto request_target = req.target();
    auto api_handler_type = RestApiHandlerType(request_target);

    switch (api_handler_type) 
    {
        case API_TYPE::GAME_JOIN: 
        {
            application::APPLICATION_ERROR join_error;
            auto response = app_.Join(req.body(), join_error);

            switch (join_error) 
            {
                case application::APPLICATION_ERROR::BAD_JSON:
                {
                    return text_response(http::status::bad_request, response);
                }
                case application::APPLICATION_ERROR::INVALID_NAME:
                {
                    return text_response(http::status::bad_request, response);
                }
                case application::APPLICATION_ERROR::MAP_NOT_FOUND:
                {
                    return text_response(http::status::not_found, response);
                }
                default:
                {
                    return text_response(http::status::ok, response);
                }
            }
        }
        case API_TYPE::MAPS_API:
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_PLAYERS: 
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_STATE:
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_PLAYER_ACTION:
        {
            auto response = SerializeMessageCode("invalidArgument", "Invalid content type");
            // check content-type
            try {
                if (req[http::field::content_type] != ContentType::APPLICATION_JSON) {
                    return text_response(http::status::bad_request, response);
                }
            }
            catch(...) {
                return text_response(http::status::bad_request, response);
            }

            // set ActionPlayer
            return SetActionPlayer(req);            
        }
        case API_TYPE::GAME_TICK:
        {
            // get state
            application::APPLICATION_ERROR app_error;
            auto response = app_.Tick(req.body(), app_error);
            switch (app_error) {
                case application::APPLICATION_ERROR::INVALID_ARGUMENT:
                {
                    return text_response(http::status::bad_request, response);
                }
                default:
                {
                    return text_response(http::status::ok, response);
                }
            }
        }
        case API_TYPE::GAME_RECORDS:
        {
            return invalid_method_get_head();
        }
        default:
        {
            auto response_status = http::status::bad_request;
            auto response = SerializeMessageCode("badRequest", "Bad request");
            return text_response(response_status, response);
        }
    }
}

Response Api::MakeUnknownMethodResponse(const StringRequest& req) {
    const auto text_response = [&](http::status status, std::string text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };
    
    const auto invalid_method_get_head = [&](){
        auto response_status = http::status::method_not_allowed;
        auto response = SerializeMessageCode("invalidMethod", "Invalid method");
        return MakeStringResponse(
            response_status, 
            response,
            req.version(), 
            req.keep_alive(),
            ContentType::APPLICATION_JSON,
            true,
            "GET, HEAD"sv);
    };

    const auto invalid_method_post = [&](){
        auto response_status = http::status::method_not_allowed;
        auto response = SerializeMessageCode("invalidMethod", "Only POST method is expected");
        return MakeStringResponse(
            response_status, 
            response,
            req.version(), 
            req.keep_alive(),
            ContentType::APPLICATION_JSON,
            true,
            "POST"sv);
    };
    
    auto request_target = req.target();
    auto api_handler_type = RestApiHandlerType(request_target);
    
    switch (api_handler_type) 
    {
        case API_TYPE::GAME_JOIN: 
        {
            return invalid_method_post();
        }
        case API_TYPE::MAPS_API:
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_PLAYERS: 
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_STATE:
        {
            return invalid_method_get_head();
        }
        case API_TYPE::GAME_PLAYER_ACTION:
        {
            return invalid_method_post();
        }
        case API_TYPE::GAME_TICK:
        {
            return invalid_method_post();
        }
        case API_TYPE::GAME_RECORDS:
        {
            return invalid_method_get_head();
        }
        default:
        {
            auto response_status = http::status::bad_request;
            auto response = SerializeMessageCode("badRequest", "Bad request");
            return text_response(response_status, response);
        }
    }
}


template <typename Fn>
StringResponse Api::ExecuteAuthorized(const StringRequest& req, Fn&& app_action) {

    const auto text_response = [&](http::status status, std::string text) {
        return MakeStringResponse(status, text, req.version(), req.keep_alive());
    };

    const auto invalid_response = [&](std::string text) {
        return MakeStringResponse(
            http::status::unauthorized, 
            text, 
            req.version(), 
            req.keep_alive(), 
            ContentType::APPLICATION_JSON);
    };

    // check auth header
    auto auth_header_it = req.find(http::field::authorization);
    if (auth_header_it == req.end())
    {
        return invalid_response(SerializeMessageCode("invalidToken", "Authorization header is required"));
    }

    // get token
    std::string auth_token{auth_header_it->value()};

    // get state
    application::APPLICATION_ERROR app_error;
    auto response = app_action(auth_token, app_error);
    switch (app_error) {
        case application::APPLICATION_ERROR::INVALID_TOKEN:
        {
            return invalid_response(SerializeMessageCode("invalidToken", "Authorization header is required"));
        }
        case application::APPLICATION_ERROR::UNKNOWN_TOKEN:
        {
            return invalid_response(SerializeMessageCode("unknownToken", "Player token has not been found"));
        }
        case application::APPLICATION_ERROR::INVALID_ARGUMENT:
        {
            return text_response(http::status::bad_request, SerializeMessageCode("invalidArgument", "Failed to parse action"));
        }
        default:
        {
            return text_response(http::status::ok, response);
        }
    }
}

StringResponse Api::GetPlayers(const StringRequest& request) {
    return ExecuteAuthorized(
        request, 
        [&](const std::string& auth_token, application::APPLICATION_ERROR& app_error){
            return app_.GetPlayers(auth_token, app_error);
        }
    );
}

StringResponse Api::SetActionPlayer(const StringRequest& request) {
    return ExecuteAuthorized(
        request, 
        [&](const std::string& auth_token, application::APPLICATION_ERROR& app_error){
            return app_.ActionPlayer(auth_token, app_error, request.body());
        }
    );
}

StringResponse Api::GetState(const StringRequest& request) {
    return ExecuteAuthorized(
        request, 
        [&](const std::string& auth_token, application::APPLICATION_ERROR& app_error){
            return app_.GetState(auth_token, app_error);
        }
    );
}

}