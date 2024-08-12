#pragma once

#include <map>
#include <string>

namespace extra_data {

class LootType {
	public:
		void SetLootType(const std::string& mapID, const std::string& loot_types, int cnt_loot_types);
		std::string GetLootType(const std::string& mapID);
		int GetCntLootType(const std::string& mapID);
	private:
		std::map<std::string, std::pair<std::string, int>> loot_types_map_;
};

}