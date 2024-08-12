#pragma once
#include <sdk.h>
#include <boost/json.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <random>
#include <model/model.h>
#include <sstream>
#include <optional>
#include <iomanip>
#include <gameplay.h>
#include <database/postgres.h>
#include <chrono>

namespace application {

namespace net = boost::asio;
using namespace std::literals;
namespace beast = boost::beast;
namespace http = beast::http;

enum APPLICATION_ERROR {
    APPLICATION_NO_ERROR,
    BAD_JSON,
    MAP_NOT_FOUND,
    INVALID_NAME,
    INVALID_TOKEN,
    UNKNOWN_TOKEN,
    INVALID_ARGUMENT
};

std::optional<std::string> check_token(const std::string& authorization_text);
std::string SerializeMessageCode(const std::string& code, const std::string& message);

class Application
{
public:
    explicit Application(model::Game& game, size_t threads_count, const std::string& database_url) : 
        game_{ game },
        database_{ threads_count, database_url } {}

    std::string GetMapJson(const std::string& request_target, http::status& response_status);
    std::string Join(const std::string& jsonBody, APPLICATION_ERROR& join_error);
    std::string GetPlayers(const std::string& auth_message, APPLICATION_ERROR& auth_error);
    std::string GetState(const std::string& auth_message, APPLICATION_ERROR& app_error);
    std::string ActionPlayer(const std::string& auth_message, APPLICATION_ERROR& app_error, const std::string& jsonBody);
    std::string Tick(const std::string& jsonBody, APPLICATION_ERROR& app_error);
    gameplay::Player* GetPlayerFromToken(const std::string& auth_message, APPLICATION_ERROR& app_error, std::string& app_error_msg);
    boost::json::array GetDogLoots(const model::Dog& dog);
    void RetirePlayers(std::chrono::milliseconds delta);
    std::string GetRecords(unsigned start, unsigned max_items, APPLICATION_ERROR& app_error);
    
    model::Game& GetGameModel() {
        return game_;
    }

    gameplay::Players& GetPlayers() {
        return players_;
    }

    gameplay::PlayerTokens& GetPlayerTokens() {
        return player_tokens_;
    }

    void SetLastPlayerId(gameplay::Player::Id last_id) {
        last_player_id_ = last_id;
    }

    gameplay::Player::Id GetLastPlayerId() const {
        return last_player_id_;
    }

private:
    model::Game& game_;
    gameplay::Players players_;
    gameplay::PlayerTokens player_tokens_;
    gameplay::Player::Id last_player_id_{0};
    postgres::Database database_;


    gameplay::Player* GetPlayer(const std::string& name, const std::string& mapId);
};
}