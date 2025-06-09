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
        USES_TERMINAL_DOWNLOAD TRUE
        GIT_PROGRESS TRUE
        DOWNLOAD_NO_EXTRACT FALSE
)
FetchContent_MakeAvailable(Boost)

add_executable(server
        server/src/main.cpp
        server/src/ServerCLI.cpp
        server/include/ServerCLI.h)

add_executable(client
        client/src/main.cpp
        client/src/PerformanceMonitor.cpp
        client/src/WSClient.cpp
)

target_include_directories(client PRIVATE client/include)

target_include_directories(server PRIVATE server/include)

target_link_libraries(client PRIVATE -lpdh -lws2_32 Boost::beast Boost::asio)

target_link_libraries(server PRIVATE -lws2_32 -lwsock32 Boost::beast Boost::asio)

