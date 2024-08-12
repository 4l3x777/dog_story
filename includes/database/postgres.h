#pragma once
//#include <pqxx/zview.hxx>
//#include <pqxx/pqxx>

#include <player_repository.h>
#include <connection_pool.h>

namespace postgres {

class RetiredPlayerRepositoryImpl : public databese_domain::RetiredPlayerRepository {
public:
    explicit RetiredPlayerRepositoryImpl(ConnectionPool& conn_pool) : conn_pool_{conn_pool} {}

    void Save(gameplay::RetiredPlayer& retired_player) override;

    std::vector<gameplay::RetiredPlayer> Get(unsigned offset, unsigned limit) override;

private:
    ConnectionPool& conn_pool_;
};


class Database {
public:
    explicit Database(size_t capacity, const std::string& db_url);

    RetiredPlayerRepositoryImpl& GetRetiredPlayerRepository() {
        return retired_players_;
    }

private:
    ConnectionPool conn_pool_;
    RetiredPlayerRepositoryImpl retired_players_{ conn_pool_ };
};

}  // namespace postgres