#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include <model.h>

using namespace std::literals;

SCENARIO("Load game model") {
	using model::Game;

	GIVEN("create game model") {
		Game game(model::LootGeneratorConfig{});
		WHEN("check maps not added") {
			CHECK(game.GetMaps().empty());
		}

	}
}