cmake_minimum_required(VERSION 3.30)
project(alprog_finpro_kelompok_iman)

set(CMAKE_CXX_STANDARD 20)

add_executable(server
        server/src/main.cpp)


add_executable(client
        client/src/main.cpp)
