#pragma once

#include <boost/serialization/vector.hpp>
#include <boost/serialization/list.hpp>

#include <model/model.h>

namespace model {

template <typename Archive>
void serialize(Archive& ar, DogSpeed& speed, [[maybe_unused]] const unsigned version) {
    ar& speed.x;
    ar& speed.y;
}

template <typename Archive>
void serialize(Archive& ar, DogCoordinate& coordinate, [[maybe_unused]] const unsigned version) {
    ar& coordinate.x;
    ar& coordinate.y;
}

template <typename Archive>
void serialize(Archive& ar, Map::Id& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj);
}

template <typename Archive>
void serialize(Archive& ar, Loot& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
    ar&(obj.coordinate);
}

template <typename Archive>
void serialize(Archive& ar, Dog::Id& obj, [[maybe_unused]] const unsigned version) {
    ar& (*obj);
}

template <typename Archive>
void serialize(Archive& ar, GameSession::Dogs& obj, [[maybe_unused]] const unsigned version) {
    for (auto &dog : obj)
        ar& (dog);
}

template <typename Archive>
void serialize(Archive& ar, model::Game::GameSessions& objs, [[maybe_unused]] const unsigned version) {
    for (auto &odj : objs)
        ar& (odj);
}

} // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetEndPos())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDirectionType())
        , score_(dog.GetScore())
        , bag_content_(dog.GetLoots())
        , lifetime_(dog.GetLifetime().count())
        , stay_time_(dog.GetStayTime().count()) {
    }

    [[nodiscard]] model::Dog Restore() {
        model::Dog dog{name_, pos_, id_};
        dog.SetSpeed(speed_);
        dog.SetDirection(direction_);
        dog.AddScore(score_);
        dog.SetLastMoveTime(std::chrono::milliseconds(lifetime_-stay_time_));
        dog.SetLifeTime(std::chrono::milliseconds(lifetime_));

        std::list<model::Loot> bag_content = bag_content_;
        for (auto it = bag_content.begin(), itn = bag_content.begin(); it != bag_content.end(); it = itn) {
            itn = std::next(it);
            dog.PickUpLoot(bag_content, it);
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_content_;
        //ar& stay_time_;
        //ar& lifetime_;
    }

private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    model::DogCoordinate pos_;
    model::DogSpeed speed_;
    model::DOG_DIRECTION direction_ {model::DOG_DIRECTION::NORTH};
    int score_ {0};
    std::list<model::Loot> bag_content_;
    size_t stay_time_;
    size_t lifetime_;

};

/* Другие классы модели сериализуются и десериализуются похожим образом */

class GameSessionRepr {
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(model::GameSession& game_session)
        : map_id_(game_session.MapId())
        , last_loot_id_(game_session.GetLastLootId())
        , last_dog_id_(game_session.GetLastDogId())
        , loots_(game_session.GetLoots())
    {
        for (auto dog : game_session.GetDogs())
            dogs_repr_.push_back(DogRepr(static_cast<model::Dog&>(dog)));
    }

    [[nodiscard]] model::GameSession Restore(
            const model::Map* map, 
            bool randomize_spawn_points,
            const model::LootGeneratorConfig loot_generator_config) const 
    {
        model::GameSession game_session(map, randomize_spawn_points, loot_generator_config);
        game_session.SetLastLootId(last_loot_id_);
        model::GameSession::Dogs dogs;
        for (auto dog_repr : dogs_repr_) {
            dogs.push_back(dog_repr.Restore());
        }
        game_session.SetDogs(dogs);
        game_session.SetLoots(loots_);
        game_session.SetLastLootId(last_loot_id_);
        game_session.SetLastDogId(last_dog_id_);
        return game_session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& map_id_;
        ar&* last_loot_id_;
        ar&* last_dog_id_;
        ar& loots_;
        ar& dogs_repr_;
    }

    [[nodiscard]] model::Map::Id GetMapId() const {
        return map_id_;
    }

private:
    model::Map::Id map_id_{std::string()};
    model::Loot::Id last_loot_id_{0};
    model::Dog::Id last_dog_id_{0};
    std::list<model::Loot> loots_;
    std::list<DogRepr> dogs_repr_;
};


class GameSessionsRepr {
public:
    GameSessionsRepr() = default;

    explicit GameSessionsRepr(const model::Game::GameSessions& game_sessions)
    {
        for (auto game_session : game_sessions)
            game_sessions_repr.push_back(GameSessionRepr(game_session));
    }

    [[nodiscard]] model::Game::GameSessions Restore(model::Game &game) const {
        model::Game::GameSessions game_sessions;
        for (auto &game_session_repr : game_sessions_repr) {
            auto map = game.FindMap(game_session_repr.GetMapId());
            game_sessions.push_back(
                game_session_repr.Restore(
                    map, 
                    game.IsRandomizeSpawnPoints(), 
                    game.GetLootGeneratorConfig()
                )
            );
        }
        return game_sessions;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr;
    }

private:
    std::list<GameSessionRepr> game_sessions_repr;
};


class GameRepr {
public:
    GameRepr() = default;

    explicit GameRepr(model::Game& game) : game_sessions_repr(game.GetGameSessions()) {}

    void Restore(model::Game &game) const {
        game.SetGameSessions(game_sessions_repr.Restore(game));
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_sessions_repr;
    }

private:
    GameSessionsRepr game_sessions_repr;
};

}  // namespace serialization
