cmake_minimum_required(VERSION 3.30)
project(alprog_finpro_kelompok_iman)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOST_INCLUDE_LIBRARIES asio beast)
set(BOOST_ENABLE_CMAKE ON)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.7z
)
FetchContent_MakeAvailable(Boost)
include(FetchContent)
FetchContent_Declare(
        nlohmann_json
        URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_executable(server
        server/src/main.cpp
        server/src/ServerCLI.cpp
        server/src/WSServer.cpp
        server/src/Session.cpp
        server/src/MetricStore.cpp)

add_executable(client
        client/src/main.cpp
        client/src/PerformanceMonitor.cpp
        client/src/WSClient.cpp
)

target_include_directories(client PRIVATE client/include)

target_include_directories(server PRIVATE server/include)

target_link_libraries(client PRIVATE -lpdh -lws2_32 Boost::beast Boost::asio nlohmann_json::nlohmann_json)

target_link_libraries(server PRIVATE -lws2_32 -lwsock32 Boost::beast Boost::asio nlohmann_json::nlohmann_json)

