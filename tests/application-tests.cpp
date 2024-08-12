#include <cmath>
#include <catch2/catch_test_macros.hpp>

#include <application.h>

using namespace std::literals;

SCENARIO("Check application::check_token function") {
	GIVEN("check_token") {    
		WHEN("not bearer auth") {
			CHECK(!application::check_token("Basic 77777777777777777777777777777777"));
		}
        WHEN("token length < 32 byte") {
            CHECK(!application::check_token("Bearer 7777777777777777777777777777777"));
        }
        WHEN("token correct1") {
            CHECK(application::check_token("Bearer 77777777777777777777777777777777"));
        }
        WHEN("token correct2") {
            CHECK(application::check_token("Bearer asdfghjklzxcvbnmqwertyuiop123456"));
        }        
	}
}