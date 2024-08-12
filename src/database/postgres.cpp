#include <postgres.h>
#include <pqxx/zview.hxx>
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include <chrono>
#include <logger/logger.h>
#include <boost/json.hpp>

namespace postgres {

using namespace std::literals;
using pqxx::operator"" _zv;
using milliseconds = std::chrono::milliseconds;

void RetiredPlayerRepositoryImpl::Save(gameplay::RetiredPlayer& retired_player) {
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};

    boost::json::object log_data;
    boost::json::object player_info;
    log_data["database"] = "save retired player";
    player_info["player_id"] = *retired_player.GetId();
    player_info["player_name"] = retired_player.GetName();
    player_info["player_score"] = retired_player.GetScore();
    player_info["player_playtime"] = retired_player.GetPlayTime().count();
    log_data["player_info"] = boost::json::serialize(player_info);
    LOG().print(
        log_data,
        "save retired player"        
    );

    work.exec_params(
        R"(
            INSERT INTO retired_players (id, name, score, play_time_ms) VALUES ($1, $2, $3, $4)
            ON CONFLICT (id) DO NOTHING;
        )"_zv,
        *retired_player.GetId(), 
        retired_player.GetName(),
        retired_player.GetScore(),
        retired_player.GetPlayTime().count()
    );
    work.commit();
}

std::vector<gameplay::RetiredPlayer> RetiredPlayerRepositoryImpl::Get(unsigned offset, unsigned limit) {
    auto conn = conn_pool_.GetConnection();

    boost::json::object log_data;
    boost::json::object info;
    log_data["database"] = "query retired players";
    info["offset"] = offset;
    info["limit"] = limit;
    log_data["info"] = boost::json::serialize(info);
    LOG().print(
        log_data,
        "Query retired players"        
    );

    pqxx::read_transaction r{*conn};
    std::vector<gameplay::RetiredPlayer> retired_players;
    auto query_text = "SELECT id, name, score, play_time_ms FROM retired_players "
        "ORDER BY score DESC, play_time_ms ASC, name ASC "
        "LIMIT " + std::to_string(limit) + " OFFSET " + std::to_string(offset) + ";";
    auto res =  r.query<size_t, std::string, unsigned, unsigned>(query_text);
    for (const auto [id, name, score, play_time] : res) {
        gameplay::RetiredPlayer retired_player(
            gameplay::Player::Id {id}, 
            name, 
            score, 
            milliseconds{play_time});
        retired_players.push_back(std::move(retired_player));
    }
    return retired_players;
}

Database::Database(size_t capacity, const std::string& db_url) 
    : conn_pool_{
        capacity, 
        [db_url] {
            return std::make_shared<pqxx::connection>(db_url);
        }
    } 
{
    auto conn = conn_pool_.GetConnection();
    pqxx::work work{*conn};
    
    boost::json::object log_data;
    log_data["database"] = "create retired players table";
    LOG().print(
        log_data,
        "Create retired_players table"        
    );

    work.exec(
        R"(
            CREATE TABLE IF NOT EXISTS retired_players (
                id bigint CONSTRAINT player_id_constraint PRIMARY KEY,
                name varchar(100) NOT NULL,
                score integer NOT NULL,
                play_time_ms integer NOT NULL
            );
        )"_zv
    );
    work.commit();
}

}  // namespace postgres