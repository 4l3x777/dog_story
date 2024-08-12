#pragma once
#include <boost/signals2.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <deque>
#include <atomic>
#include <map>
#include <memory>
#include <list>
#include <random>

#include <tagged.h>
#include <loot_generator.h>
#include <extra_data.h>

namespace model {

class GameSession;

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name) noexcept
        : id_(std::move(id))
        , name_(std::move(name)) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(const Office& office);

    void SetDogSpeed(double dog_speed) {
        dog_speed_ = dog_speed;
    }

    double GetDogSpeed() const {
        return dog_speed_;
    }

    void SetBagCapacity(int bag_capacity) {
        bag_capacity_ = bag_capacity;
    }

    int GetBagCapacity() const {
        return bag_capacity_;
    } 

    void AddLootScore(int loot_score) {
        loot_scores_.push_back(loot_score);
    }

    const std::vector<int>& GetLootScores() {
        return loot_scores_;
    }

    int GetLootTypesCount() {
        return loot_scores_.size();
    }

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    double dog_speed_;
    int bag_capacity_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
    std::vector<int> loot_scores_;
};

class LootGeneratorConfig {
public:    
    std::chrono::milliseconds period;
    double probability {0.5};
};

class Game {
public:
    Game(const LootGeneratorConfig& loot_generator_config) : loot_generator_config_(std::move(loot_generator_config)) {}

    using Maps = std::vector<Map>;

    using GameSessions = std::deque<GameSession>;

    void AddMap(const Map& map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    void SetDefaultDogSpeed(double speed) {
        DefaultDogSpeed = speed;
    }

    void SetDefaultBagCapacity(int bag_capacity) {
        DefaultBagCapacity = bag_capacity;
    }

    const Map* FindMap(const Map::Id& id) const noexcept;

    GameSession* FindGameSession(const Map::Id& id) noexcept;

    GameSession* AddGameSession(const Map::Id& id);

    void Tick(uint64_t time_delta);

    void SetRandomizeSpawnPoints(bool randomize_spawn_points) {
        randomize_spawn_points_ = randomize_spawn_points;
    }

    bool IsRandomizeSpawnPoints() {
        return randomize_spawn_points_;
    }

    std::string GetLootType(const Map::Id& id) {
        return loot_type_.GetLootType(*id);
    }

    void AddLootType(const extra_data::LootType& loot_type) {
        loot_type_ = std::move(loot_type);
    }

    GameSessions& GetGameSessions() {
        return game_sessions_;
    }

    void SetGameSessions(const GameSessions& game_sessions);

    const LootGeneratorConfig& GetLootGeneratorConfig() const {
        return loot_generator_config_;
    }

    // Добавляем обработчик сигнала tick и возвращаем объект connection для управления,
    // при помощи которого можно отписаться от сигнала
    [[nodiscard]] boost::signals2::connection DoOnTickSlot(const boost::signals2::signal<void(std::chrono::milliseconds delta)>::slot_type& handler) {
        return tick_signal_.connect(handler);
    }

    void SetRetirementTime(double retirement_time) {
        retirement_time_ = std::chrono::milliseconds(static_cast<size_t>(retirement_time));
    }

    std::chrono::milliseconds GetRetirementTime() const {
        return retirement_time_;
    }

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
     
    Maps maps_;
    double DefaultDogSpeed {0.0};
    int DefaultBagCapacity {0};
    GameSessions game_sessions_;
    MapIdToIndex map_id_to_index_;
    MapIdToIndex map_id_to_game_sessions_index_;
    bool randomize_spawn_points_ {false};
    LootGeneratorConfig loot_generator_config_;
    extra_data::LootType loot_type_;
    boost::signals2::signal<void(std::chrono::milliseconds delta)> tick_signal_;
    std::chrono::milliseconds retirement_time_{0};
};


class DogCoordinate {
public:
    double x{0};
    double y{0};

    bool operator==(const DogCoordinate& dog) const {
        if (x == dog.x && y == dog.y) {
            return true;
        }
        else {
            return false;
        }
    }

    bool operator!=(const DogCoordinate& dog) const {
        if (x != dog.x || y != dog.y) {
            return true;
        }
        else {
            return false;
        }        
    }
};

class Loot {
public:
    using Id = util::Tagged<uint32_t, Loot>;

    Id id{0};
    int type {0};
    DogCoordinate coordinate;
};

class DogSpeed {
public:
    double x{0};
    double y{0};

    bool operator==(const DogSpeed& dog_speed) const {
        if (x == dog_speed.x && y == dog_speed.y) {
            return true;
        }
        else {
            return false;
        }
    }
};

enum DOG_DIRECTION {
    NORTH,
    SOUTH,
    WEST,
    EAST
};

enum DOG_MOVE {
    LEFT,
    RIGHT,
    UP,
    DOWN,
    STAND
};

#define DOG_INDEX() DogIndexer::GetInstance().GetIndex()
#define UPDATE_DOG_INDEX(id) DogIndexer::GetInstance().UpdateIndex(id)
class DogIndexer {
    DogIndexer() = default;
    DogIndexer(const DogIndexer&) = delete;
public:
    static DogIndexer& GetInstance() {
        static DogIndexer obj;
        return obj;
    }

    size_t GetIndex() {
        return index++;
    }

    void UpdateIndex(size_t current_index) {
        if (index < current_index) {
            index = current_index + 1;
        }
    }

private:
    std::atomic<size_t> index{0};
};

class Dog {
public:
    using Id = util::Tagged<size_t, Dog>;
    Dog(
        const std::string& dog_name,
        const DogCoordinate& dog_coordinate
    ) : 
        dog_name_(dog_name), 
        id_(Id{ DOG_INDEX() }),  
        dog_coordinate_(dog_coordinate) {}
    
    const Id& GetId() {
        return id_;
    }

    std::string GetName() {
        return dog_name_;
    }

    const DogCoordinate& GetCoordinate() {
        return dog_coordinate_;
    }

    const DogCoordinate& GetEndPos() const {
        return dog_coordinate_;
    }
    const DogCoordinate& GetStartPos() const {
        return prev_dog_coordinate_;
    }

    void PickUpLoot(std::list<Loot>& session_loots, const auto& it_loot) {
        loots_.splice(loots_.end(), session_loots, it_loot);
    }

    std::list<Loot>& GetLoots() {
        return loots_;
    }

    void ReturnLoots(const std::vector<int>& loot_scores) {
        for (const auto& loot : loots_) {
            score_ += loot_scores[loot.type];
        }
        return loots_.clear();
    }

    void SetCoordinate(const DogCoordinate& coordinate) {
        std::swap(prev_dog_coordinate_, dog_coordinate_);
        
        dog_coordinate_.x = coordinate.x;
        dog_coordinate_.y = coordinate.y;
    }

    const DogSpeed& GetSpeed() {
        return dog_speed_;
    }

    void SetSpeed(const DogSpeed& speed) {
        dog_speed_ = speed;
    }

    std::string GetDirection();

    DOG_DIRECTION GetDirectionType() {
        return dog_direction_;
    }

    void SetDirection(DOG_DIRECTION dog_direction) {
        dog_direction_ = dog_direction;
    }

    void Direction(DOG_MOVE dog_move, double speed);

    DogCoordinate GetEndCoordinate(uint64_t move_time_ms);

    bool IsStanding();

    void StopMove() {
        dog_speed_.x = 0;
        dog_speed_.y = 0;
    }

    int GetScore() const {
        return score_;
    }

    void AddScore(int score) {
        score_ = score;
    }

    std::chrono::milliseconds GetStayTime() const {
        return lifetime_-last_move_time_;
    }

    std::chrono::milliseconds GetLifetime() const {
        return lifetime_;
    }

    void IncLifetime(std::chrono::milliseconds time_delta) {
        lifetime_ += time_delta;
        if (dog_speed_ != DogSpeed{0, 0}) {
            last_move_time_ = lifetime_;
        }
    }

    void SetLastMoveTime(std::chrono::milliseconds time) {
        last_move_time_ = time;
    }

    void SetLifeTime(std::chrono::milliseconds time) {
        lifetime_ = time;
    }

    Dog& operator=(const Dog& other);

    Dog(const Dog& dog) : 
        id_(dog.id_), 
        dog_name_(dog.dog_name_), 
        dog_coordinate_(dog.dog_coordinate_),
        dog_speed_(dog.dog_speed_),
        dog_direction_(dog.dog_direction_) {UPDATE_DOG_INDEX(*dog.id_);}

    Dog(Dog&& dog) : 
        id_(std::move(dog.id_)),
        dog_name_(std::move(dog.dog_name_)),
        dog_coordinate_(std::move(dog.dog_coordinate_)),
        dog_speed_(std::move(dog.dog_speed_)),
        dog_direction_(std::move(dog.dog_direction_)) {UPDATE_DOG_INDEX(*dog.id_);}

     Dog(const std::string& name, const DogCoordinate& coordinate, Id id) :
        dog_name_(name), 
        dog_coordinate_(coordinate),
        id_(id) {UPDATE_DOG_INDEX(*id);}

private:
    Id id_ = Id{0};
    std::string dog_name_;
    DogCoordinate dog_coordinate_;
    DogSpeed dog_speed_{0, 0};
    DOG_DIRECTION dog_direction_ {DOG_DIRECTION::NORTH};
    DogCoordinate prev_dog_coordinate_;
    std::list<Loot> loots_;
    int score_ {0};
    std::chrono::milliseconds last_move_time_{0};
    std::chrono::milliseconds lifetime_{0};
};


class KeyHash {
public:
    template<typename T>
    size_t operator()(const T& k) const {
        return std::hash<decltype(k.x)>()(k.x) ^ (std::hash<decltype(k.y)>()(k.y) << 1);
    }
};

class KeyEqual {
public:
    template<typename T>
    bool operator()(const T& lhs, const T& rhs) const {
        return lhs.x == rhs.x && lhs.y == rhs.y;
    }
};

class GameSession {
    using RoadMap = std::unordered_multimap<Point, const Road*, KeyHash, KeyEqual>;
    using RoadMapIter = decltype(RoadMap{}.equal_range(Point{}));
public:
    using Dogs = std::deque<Dog>;
    using Loots = std::list<Loot>;

    GameSession(
        const Map* map, 
        bool randomize_spawn_points,
        const LootGeneratorConfig& loot_generator_config
        ) : 
        map_(map),
        randomize_spawn_points_ (randomize_spawn_points),
        loot_generator_(
            loot_generator_config.period, 
            loot_generator_config.probability,
            [&]() {
                std::random_device random_device_;
                auto random_coordinate = [&](auto x, auto y) {
                        if (y < x) {
                            std::swap(x, y);
                        }
                        std::uniform_real_distribution<double> dist(x, y);
                        return dist(random_device_); 
                };
                return random_coordinate(0.0, 1.0);
            }
        ) 
    {
            LoadRoadMap();
    }

    const Loots& GetLoots() const {
        return loots_;
    }

    const Loot::Id& GetLastLootId() const {
        return loot_id_;
    }

    Loot::Id GetNextLootId() {
        return Loot::Id{(*loot_id_)++};
    }

    void SetLastLootId(const Loot::Id& loot_id) {
        loot_id_ = loot_id;
    }

    void SetLoots(const Loots& loots) {
        loots_ = loots;
    }

    const Map::Id& MapId() {
        return map_->GetId();
    } 

    Dog* FindDog(const std::string& nick_name);

    Dog* FindDog(Dog::Id dog_id)
    {
        for (auto& dog : dogs_) {
            if (*dog.GetId() == *dog_id)
                return &dog;
        }
        return nullptr;
    }

    Dog* AddDog(const std::string& nick_name);

    DogCoordinate GetRandomRoadCoordinate();

    const Dogs& GetDogs() const {
        return const_cast<Dogs&>(dogs_);
    }

    void MoveDogToContainerAndIndexing(Dog&& dog) {
        const size_t index = dogs_.size();
        auto &o = dogs_.emplace_back(std::move(dog));
        try {
            dogs_id_to_index_.emplace(o.GetId(), index);
        }
        catch (...) {
            dogs_.pop_back();
            throw std::bad_alloc();
        }
    }

    void SetDogs(const Dogs& dogs) {
        for (auto it = dogs.begin(); it != dogs.end(); ++it) {
            auto dog = *it;
            MoveDogToContainerAndIndexing(std::move(dog));
        }
    }

    const Dog::Id& GetLastDogId() const {
        return dog_id_;
    }

    void SetLastDogId(const Dog::Id& dog_id) {
        dog_id_ = dog_id;
    }

    void MoveDog(const Dog::Id& dog_id, const DOG_MOVE& dog_move);

    void Tick(uint64_t time_delta);

    DogCoordinate MoveDog(const DogCoordinate& start_coordinate, const DogCoordinate& end_coordinate);

    bool IsCoordinateOnRoad(const RoadMapIter& roads, const DogCoordinate& coordinate);

    DogCoordinate FindBorderCoordinate(const RoadMapIter& roads, const DogCoordinate& end_coordinate);

    void LoadRoadMap();

    void PickUpAndReturnLoots();

    void PushLootsToMap(std::chrono::milliseconds time_delta_ms);

    void DeleteDog(const Dog::Id& dog_id);

private:
    using DogsIdHasher = util::TaggedHasher<Dog::Id>;
    using DogsIdToIndex = std::unordered_map<Dog::Id, size_t, DogsIdHasher>;
    Dogs dogs_;
    DogsIdToIndex dogs_id_to_index_;
    const Map* map_;
    RoadMap road_map_;
    bool randomize_spawn_points_;
    loot_gen::LootGenerator loot_generator_;
    Loots loots_;
    Loot::Id loot_id_ {0};
    Dog::Id dog_id_{0};
};



}  // namespace model
