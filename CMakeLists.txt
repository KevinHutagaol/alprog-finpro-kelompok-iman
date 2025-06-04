cmake_minimum_required(VERSION 3.30)
project(alprog_finpro_kelompok_iman)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

target_link_libraries(client PRIVATE -lpdh)
