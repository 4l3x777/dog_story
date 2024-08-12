#include <model/extra_data.h>
#include <iostream>

namespace extra_data {

void LootType::SetLootType(const std::string& mapID, const std::string& loot_types, int cnt_loot_types)
{
	loot_types_map_[mapID] = std::make_pair(loot_types, cnt_loot_types);
}

std::string LootType::GetLootType(const std::string& mapID) {
	auto it = loot_types_map_.find(mapID);
	if (it == loot_types_map_.end()) {
		throw std::runtime_error(__FUNCTION__ + std::string(": map not found"));
    }
	return it->second.first;
}

int LootType::GetCntLootType(const std::string& mapID) {
	auto it = loot_types_map_.find(mapID);
	if (it == loot_types_map_.end()) {
		throw std::runtime_error(__FUNCTION__ + std::string(": map not found"));
    }
	return it->second.second;
}

}