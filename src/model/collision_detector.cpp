#include "collision_detector.h"
#include <cassert>
#include <algorithm>
#include <vector>

namespace collision_detector {

CollectionResult TryCollectPoint(geom::Point2D a, geom::Point2D b, geom::Point2D c) {
    // Проверим, что перемещение ненулевое.
    // Тут приходится использовать строгое равенство, а не приближённое,
    // пскольку при сборе заказов придётся учитывать перемещение даже на небольшое
    // расстояние.
    assert(b.x != a.x || b.y != a.y);
    const double u_x = c.x - a.x;
    const double u_y = c.y - a.y;
    const double v_x = b.x - a.x;
    const double v_y = b.y - a.y;
    const double u_dot_v = u_x * v_x + u_y * v_y;
    const double u_len2 = u_x * u_x + u_y * u_y;
    const double v_len2 = v_x * v_x + v_y * v_y;
    const double proj_ratio = u_dot_v / v_len2;
    const double sq_distance = u_len2 - (u_dot_v * u_dot_v) / v_len2;

    return CollectionResult(sq_distance, proj_ratio);
}


std::vector<GatheringEvent> FindGatherEvents(const ItemGathererProvider& provider) {
    std::vector<GatheringEvent> gather_events;

    auto equal_positions = [](geom::Point2D p1, geom::Point2D p2) {
        return p1.x == p2.x && p1.y == p2.y;
    };

    for (auto gatherer_id = 0; gatherer_id < provider.GatherersCount(); ++gatherer_id) {
        auto gatherer = provider.GetGatherer(gatherer_id);
        
        // check gatherer start == end
        if (equal_positions(gatherer.start_pos, gatherer.end_pos)) {
            continue;
        }
        for (auto item_id = 0; item_id < provider.ItemsCount(); ++item_id) {
            auto item = provider.GetItem(item_id);
            auto collect_result = TryCollectPoint(gatherer.start_pos, gatherer.end_pos, item.position);

            if (collect_result.IsCollected(gatherer.width + item.width)) {
                GatheringEvent gather_event { 
                    .item_id = item_id,
                    .gatherer_id = gatherer_id,
                    .sq_distance = collect_result.sq_distance,
                    .time = collect_result.proj_ratio
                };
                gather_events.push_back(gather_event);
            }
        }
    }

    std::sort(
        gather_events.begin(), 
        gather_events.end(),
        [] (const GatheringEvent& event1, const GatheringEvent& event2) {
                return event1.time < event2.time;
        }
    );

    return gather_events;
}


}  // namespace collision_detector