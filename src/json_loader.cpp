#include <json_loader.h>
#include <iostream>
#include <fstream>
#include <optional>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/json.hpp>

namespace json_loader {

using namespace boost::property_tree;

static constexpr size_t default_bag_capacity = 3;
static constexpr double default_retirement_time = 60;

model::Road LoadRoad(const ptree &road) {
    try {
        model::Point point_start;
        point_start.x = road.get<int>("x0");
        point_start.y = road.get<int>("y0");
        if (road.to_iterator(road.find("y1")) == road.end()) {
            return model::Road(model::Road::HORIZONTAL, point_start, road.get<int>("x1"));
        }
        else {
            return model::Road(model::Road::VERTICAL, point_start, road.get<int>("y1"));
        }
    }
    catch (std::exception& e) {
        throw std::runtime_error("Error loading road " + std::string(e.what()));
    }
}

model::Building LoadBuilding(const ptree& building) {
    try {
        auto rectangle = model::Rectangle(
            {
                .position = {
                    .x = building.get<int>("x"),
                    .y = building.get<int>("y")
                },
                .size = {
                    .width = building.get<int>("w"),
                    .height = building.get<int>("h")
                }
            }
        );

        return model::Building(rectangle);
    }
    catch (std::exception& e) {
        throw std::runtime_error("Error loading building " + std::string(e.what()));
    }
}

model::Office LoadOffice(const ptree& office) {
    try {
        auto office_id = model::Office::Id(office.get<std::string>("id"));
        return model::Office(office_id, 
            model::Point{.x = office.get<int>("x"), .y = office.get<int>("y")}, 
            model::Offset{.dx = office.get<int>("offsetX"), .dy = office.get<int>("offsetY")});
    }
    catch (std::exception& e) {
        throw std::runtime_error("Error loading office " + std::string(e.what()));
    }
}

model::Map LoadMap(const ptree& map, double defaultDogSpeed, int defaultBagCapacity) {
    try {
        auto map_id = model::Map::Id(map.get<std::string>("id"));
        auto map_name = map.get<std::string>("name");
        model::Map model_map(map_id, map_name);

        // check dog speed on map
        if (map.to_iterator(map.find("dogSpeed")) != map.end()) {
            model_map.SetDogSpeed(map.get<double>("dogSpeed"));
        }
        else {
            model_map.SetDogSpeed(defaultDogSpeed);
        }

        // check bag capacity on map
        if (map.to_iterator(map.find("bagCapacity")) != map.end()) {
            model_map.SetBagCapacity(map.get<int>("bagCapacity"));
        }
        else {
            model_map.SetBagCapacity(defaultBagCapacity);
        }

        auto roads = map.get_child("roads");
        for (auto road : roads) {
            model_map.AddRoad(LoadRoad(road.second));
        }
        auto buildings = map.get_child("buildings");
        for (auto building : buildings) {
            model_map.AddBuilding(LoadBuilding(building.second));
        }
        auto offices = map.get_child("offices");
        for (auto office : offices) {
            model_map.AddOffice(LoadOffice(office.second));
        }
        auto loots = map.get_child("lootTypes");
        for (auto loot : loots) {
            model_map.AddLootScore(loot.second.get<int>("value"));
        }
        return model_map;
    }
    catch (std::exception& e) {
        throw std::runtime_error("Error loading map " + std::string(e.what()));
    }
}

model::LootGeneratorConfig LoadLootGeneratorConfig(const ptree& ptree) {
    auto loot_conf = ptree.get_child("lootGeneratorConfig");
    model::LootGeneratorConfig lgc;
    lgc.period = std::chrono::milliseconds(static_cast<int>(loot_conf.get<double>("period") * 1000.0));
    lgc.probability = loot_conf.get<double>("probability");
    return lgc;
}

void LoadLootType(model::Game &game, const std::filesystem::path& json_path) {
    extra_data::LootType loot_type;
    boost::json::error_code ec;
    std::string json_string;
    std::ifstream json_file(json_path);
    if (!json_file.is_open()) {
        throw std::runtime_error(__FUNCTION__ + std::string(": json file not found"));
    }

    std::stringstream buffer;
    buffer << json_file.rdbuf();
    json_string = buffer.str();
    boost::json::value value = boost::json::parse(json_string, ec);
    if (ec) {
        throw std::runtime_error(__FUNCTION__ + std::string(": ") + ec.what());
    }

    auto json_maps = value.at("maps");
    for (auto& json_map : json_maps.get_array()) {
        auto id = json_map.at("id").as_string();
        auto json_loot_types = json_map.at("lootTypes");
        
        size_t cnt = json_loot_types.as_array().size();
        if (cnt < 1) {
            throw std::runtime_error(__FUNCTION__ + std::string(": map must contains at least one item"));
        }

        loot_type.SetLootType(
            id.c_str(), 
            boost::json::serialize(json_loot_types), 
            static_cast<int>(cnt)
        );
    }
    game.AddLootType(loot_type);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла

    std::ifstream jsonFile(json_path);
    if (!jsonFile) {
        throw std::runtime_error("Error opening file " + json_path.string());
    }

    try {
        ptree pt;
        json_parser::read_json(jsonFile, pt);

        model::Game game(LoadLootGeneratorConfig(pt));

        // check default dog speed global
        auto defaultDogSpeed = pt.get<double>("defaultDogSpeed");
        game.SetDefaultDogSpeed(defaultDogSpeed);

        // check default bag capacity
        auto defaultBagCapacity = pt.get<int>("defaultBagCapacity", default_bag_capacity);

        // check default retirement time
        auto defaultRetirementTime = pt.get<double>("dogRetirementTime", default_retirement_time) * 1000;
        game.SetRetirementTime(defaultRetirementTime);

        auto maps = pt.get_child("maps");
        for (auto map : maps) {
            game.AddMap(LoadMap(map.second, defaultDogSpeed, defaultBagCapacity));
        }

        LoadLootType(game, json_path);

        return game;
    } 
    catch (ptree_error &e) {
        throw std::runtime_error("Json file read error: "  + std::string(e.what()));
    }
}

}  // namespace json_loader
