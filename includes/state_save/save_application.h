#pragma once

#include <application/application.h>
#include <boost/serialization/map.hpp>
#include <application/gameplay.h>
#include <application/application.h>
#include <save_model.h>


namespace gameplay {

    template <typename Archive>
    void serialize(Archive& ar, Player::Id& player, [[maybe_unused]] const unsigned version) {
        ar& (*player);
    }
} // namespace application

namespace serialization {

class PlayerRepr {
public:
    PlayerRepr() = default;

    explicit PlayerRepr(gameplay::Player* player)
        : player_id_(player->GetId())
        , game_session_id_(player->GetSession()->MapId())
        , dog_id_(player->GetDog()->GetId())
    {        
    }

    void Restore(application::Application& application) const {
        model::GameSession* session = application.GetGameModel().FindGameSession(game_session_id_);
        model::Dog* dog = session->FindDog(dog_id_);
        application.GetPlayers().Add(dog, session, player_id_);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_id_;
        ar& game_session_id_;
        ar& dog_id_;
    }

private:
    gameplay::Player::Id player_id_{0};
    model::Map::Id game_session_id_{""};
    model::Dog::Id dog_id_{0};
};

class PlayersRepr {
public:
    PlayersRepr() = default;

    explicit PlayersRepr(const gameplay::Players& players) {
        for(auto& player : players.GetPlayers()) 
            players_repr_.push_back(std::move(PlayerRepr(player.get())));
    }

    void Restore(application::Application& application) const {
        for (auto& player_repr : players_repr_) {
            player_repr.Restore(application);
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& players_repr_;
    }

private:
    std::list<PlayerRepr> players_repr_;
};

class PlayerTokensRepr {
public:
    PlayerTokensRepr() = default;

    explicit PlayerTokensRepr(const gameplay::PlayerTokens& player_tokens)
    {
        for (auto& player_token : player_tokens.GetTokens()) {
            player_tokens_[*player_token.first] = *player_token.second->GetId();
        }
    }

    void Restore(application::Application& application) const {
        for (auto& player_token : player_tokens_) {
            application.GetPlayerTokens().AddToken(
                gameplay::Token{ player_token.first },
                application.GetPlayers().FindPlayer(gameplay::Player::Id{ player_token.second })
            );
        }
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& player_tokens_;
    }

private:
    std::map<std::string, uint64_t> player_tokens_;
};

class ApplicationRepr {
public:
    ApplicationRepr() = default;

    explicit ApplicationRepr(application::Application& application)
        : game_repr_(application.GetGameModel())
        , players_repr_(application.GetPlayers())
        , player_tokens_repr_(application.GetPlayerTokens()) 
        , last_player_id_(application.GetLastPlayerId())
    {}

    void Restore(application::Application& application) const {
        game_repr_.Restore(application.GetGameModel());
        players_repr_.Restore(application);
        player_tokens_repr_.Restore(application);
        application.SetLastPlayerId(last_player_id_);
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& game_repr_;
        ar& players_repr_;
        ar& player_tokens_repr_;
        ar& last_player_id_;
    }

private:
    GameRepr game_repr_;
    PlayersRepr players_repr_;
    PlayerTokensRepr player_tokens_repr_;
    gameplay::Player::Id last_player_id_{0};
};

}  // namespace serialization