#include <gameplay.h>
#include <algorithm>

namespace gameplay {

std::string ModelJsonSerializer::SerializeMaps()
{
    auto& maps = game_.GetMaps();
    boost::json::array maps_array;
    for (auto& map : maps) {
        boost::json::object map_object;
        map_object["id"] = *map.GetId();
        map_object["name"] = map.GetName();
        maps_array.emplace_back(map_object);
    }
    return boost::json::serialize(maps_array);
}

boost::json::array ModelJsonSerializer::SerializeRoads(const model::Map* map)
{
    boost::json::array array;
    for (auto& road : map->GetRoads()) {
        boost::json::object map_road;
        map_road["x0"] = road.GetStart().x;
        map_road["y0"] = road.GetStart().y;
        if (road.IsHorizontal()) map_road["x1"] = road.GetEnd().x;
        else map_road["y1"] = road.GetEnd().y;
        array.emplace_back(map_road);
    }
    return array;
}

boost::json::array ModelJsonSerializer::SerializeBuildings(const model::Map* map)
{
    boost::json::array array;
    for (auto& building : map->GetBuildings()) {
        boost::json::object map_building;
        map_building["x"] = building.GetBounds().position.x;
        map_building["y"] = building.GetBounds().position.y;
        map_building["w"] = building.GetBounds().size.width;
        map_building["h"] = building.GetBounds().size.height;
        array.emplace_back(map_building);
    }
    return array;
}

boost::json::array ModelJsonSerializer::SerializeOffices(const model::Map* map)
{
    boost::json::array array;
    for (auto office : map->GetOffices()) {
        boost::json::object map_office;
        map_office["id"] = *office.GetId();
        map_office["x"] = office.GetPosition().x;
        map_office["y"] = office.GetPosition().y;
        map_office["offsetX"] = office.GetOffset().dx;
        map_office["offsetY"] = office.GetOffset().dy;
        array.emplace_back(map_office);
    }
    return array;
}

std::optional<std::string> ModelJsonSerializer::SerializeMap(const std::string& map_name)
{
    auto map_id = model::Map::Id(map_name);
    auto map = game_.FindMap(map_id);
    if (map == nullptr) return std::nullopt;
    boost::json::object map_object;
    map_object["id"] = *map->GetId();
    map_object["name"] = map->GetName();
    map_object["roads"] = SerializeRoads(map);
    map_object["buildings"] = SerializeBuildings(map);
    map_object["offices"] = SerializeOffices(map);

    boost::system::error_code ec;
    auto value = boost::json::parse(game_.GetLootType(map->GetId()), ec);
    if(ec) {
        throw std::runtime_error(__FUNCTION__ + std::string(": loot type incorrent format"));
    }
    map_object["lootTypes"] = value;

    return boost::json::serialize(map_object);
}

void Player::Move(const std::string& command) {
    model::DOG_MOVE dog_move;
    if (command == "L") 
    {
        dog_move = model::DOG_MOVE::LEFT;
    }
    else if (command == "R")
    {
        dog_move = model::DOG_MOVE::RIGHT;
    }
    else if (command == "U")
    {
        dog_move = model::DOG_MOVE::UP;
    }
    else if (command == "D")
    {
        dog_move = model::DOG_MOVE::DOWN;
    }
    else 
    {
        dog_move = model::DOG_MOVE::STAND;
    }
    session_->MoveDog(dog_->GetId(), dog_move);    
}

Player* PlayerTokens::FindPlayer(const Token& token)
{
    if (token_to_player.contains(token)) {
        return token_to_player.at(token);
    }
    return nullptr;
}

Token PlayerTokens::AddPlayer(Player* player)
{
    Token token{GetToken()};
    token_to_player[token] = player;
    return token;
}

void PlayerTokens::DeletePlayerToken(const Player::Id& player_id) {
    for (const auto& [token, player] : token_to_player) {
        if (player->GetId() == player_id) {
            token_to_player.erase(token);
            return;
        }
    }
}

Player* Players::AddPlayer(auto&& player) {
    auto index = players_.size();
    if (auto [it, inserted] = player_id_to_index_.emplace(player->GetId(), index); !inserted) {
        throw std::invalid_argument("Player with id "s + std::to_string(*player->GetId()) + " already exists"s);
    }
    else {
        try {
            players_.emplace_back(std::move(player));
        }
        catch (...) {
            player_id_to_index_.erase(it);
            throw;
        }
    }
    return players_.back().get();
}

Player* Players::Add(model::Dog* dog, model::GameSession* session) {
    std::unique_ptr<Player> player = std::make_unique<Player>(session, dog);
    return AddPlayer(std::move(player));
}

Player* Players::Add(model::Dog *dog, model::GameSession *session, Player::Id player_id) {
    std::unique_ptr<Player> player = std::make_unique<Player>(session, dog, player_id);
    return AddPlayer(std::move(player));
}


Player* Players::FindPlayer(Player::Id player_id, model::Map::Id map_id) {
    if (auto it = player_id_to_index_.find(player_id); it != player_id_to_index_.end()) {
        auto pl = players_.at(it->second).get();
        if (pl->MapId() == map_id) {
            return pl;
        }
    }
    return nullptr;
}

const Player::Id* Players::FindPlayerId(std::string player_name, model::Map::Id map_id)
{
    auto check_player_condition = [&](const auto& player) {
        if (player->GetName() == player_name && player->MapId() == map_id) {
            return true;
        }
        else {
            return false;
        }
    };

    auto palyer = std::find_if(begin(players_), end(players_), check_player_condition);
    if (palyer == end(players_)) {
        return nullptr;
    }
    else {
        return &(*palyer)->GetId();
    }
}

Player* Players::FindPlayer(Player::Id player_id)
{
    if (auto it = player_id_to_index_.find(player_id); it != player_id_to_index_.end()) {
        auto pl = players_.at(it->second).get();
        return pl;
    }
    return nullptr;
}

void Players::DeletePlayer(const Player::Id& player_id) {
    for (auto player_it = players_.begin(); player_it != players_.end(); ++player_it) {
            if ((**player_it).GetId() == player_id) {
                players_.erase(player_it);
                return;
            }
    }
}

};