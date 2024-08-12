#pragma once
#include <string>
#include <vector>
#include <tagged.h>
#include <application/gameplay.h>

namespace databese_domain {


class RetiredPlayerRepository {
public:
    virtual void Save(gameplay::RetiredPlayer& player) = 0;
    virtual std::vector<gameplay::RetiredPlayer> Get(unsigned offset, unsigned limit) = 0;

protected:
    virtual ~RetiredPlayerRepository() = default;
};

}  // namespace databese_domain