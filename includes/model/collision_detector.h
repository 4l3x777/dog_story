#pragma once

#include "geom.h"

#include <algorithm>
#include <vector>

namespace collision_detector {

struct CollectionResult {
    bool IsCollected(double collect_radius) const {
        return proj_ratio >= 0 && proj_ratio <= 1 && sq_distance <= collect_radius * collect_radius;
    }

    // квадрат расстояния до точки
    double sq_distance;

    // доля пройденного отрезка
    double proj_ratio;
};

// Движемся из точки a в точку b и пытаемся подобрать точку c.
// Эта функция реализована в уроке.
CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c);

struct Item {
    geom::Point2D position;
    double width;
};

struct Gatherer {
    geom::Point2D start_pos;
    geom::Point2D end_pos;
    double width;
};

class ItemGathererProvider {
protected:
    ~ItemGathererProvider() = default;

public:
    virtual size_t ItemsCount() const = 0;
    virtual Item GetItem(size_t idx) const = 0;
    virtual size_t GatherersCount() const = 0;
    virtual Gatherer GetGatherer(size_t idx) const = 0;
};

struct GatheringEvent {
    size_t item_id;
    size_t gatherer_id;
    double sq_distance;
    double time;
};

class ItemGatherer : public ItemGathererProvider {
public:
    using Items = std::vector<Item>;
    using Gatherers = std::vector<Gatherer>;

    void Set(const Gatherers& gatherers) {
        gatherers_ = gatherers;
    }
    void Set(const Items& items) {
        items_ = items;
    }
    void Add(Item&& item) {
        items_.push_back(std::move(item));
    }
    void Add(Gatherer&& gatherer) {
        gatherers_.push_back(std::move(gatherer));
    }
    virtual size_t ItemsCount() const override {
        return items_.size();
    }
    virtual Item GetItem(size_t idx) const override {
        return items_[idx];
    }
    virtual size_t GatherersCount() const override {
        return gatherers_.size();
    }
    virtual Gatherer GetGatherer(size_t idx) const override {
        return gatherers_[idx];
    }

private:
    Items items_;
    Gatherers gatherers_;
};

// Эту функцию вам нужно будет реализовать в соответствующем задании.
// При проверке ваших тестов она не нужна - функция будет линковаться снаружи.
std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider);

}  // namespace collision_detector