cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
conan_basic_setup(TARGETS)

include_directories(includes 
	includes/logger 
	includes/network 
	includes/network/rest_api 
	includes/model 
	includes/application 
	includes/state_save
	includes/database
)

add_library(logger STATIC
	src/logger/logger.cpp
)

target_link_libraries(logger PUBLIC CONAN_PKG::boost)

add_library(database STATIC
	src/database/player_repository.cpp
	src/database/postgres.cpp
)

target_link_libraries(database PUBLIC CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx application logger)

add_library(application STATIC
	src/application/application.cpp
	src/application/gameplay.cpp
)

target_link_libraries(application PUBLIC CONAN_PKG::boost model database)

add_library(model STATIC
	src/model/loot_generator.cpp
	src/model/extra_data.cpp
	src/model/model.cpp
	src/model/collision_detector.cpp
)

target_link_libraries(model PUBLIC CONAN_PKG::boost)

add_library(state_save STATIC
	src/state_save/serializing_listener.cpp
)

target_link_libraries(state_save PUBLIC CONAN_PKG::boost application model logger)

add_executable(game_server
	src/network/http_server.cpp
	src/network/request_handler.cpp
	src/network/rest_api/api.cpp
	src/network/rest_api/file.cpp
	src/network/rest_api/response_base.cpp
	src/json_loader.cpp
	src/ticker.cpp
	src/main.cpp
)

target_link_libraries(game_server PRIVATE CONAN_PKG::boost logger model application state_save)

add_executable(game_server_tests
    tests/model-tests.cpp
    tests/loot-generator-tests.cpp
	tests/collision-detector-tests.cpp
	tests/application-tests.cpp
)

target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost model application)