cmake_minimum_required(VERSION 3.30)
project(alprog_finpro_kelompok_iman)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mthreads")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")


#set(BOOST_INCLUDE_LIBRARIES asio beast system coroutine)
set(BOOST_ENABLE_CMAKE ON)

find_package(Threads REQUIRED)

#add_compile_definitions(BOOST_THREAD_POSIX)

include(FetchContent)
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
        Boost
        URL https://github.com/boostorg/boost/releases/download/boost-1.84.0/boost-1.84.0.7z
#        CMAKE_ARGS -DBOOST_THREAD_USE_POSIX_MUTEX=ON
)
FetchContent_MakeAvailable(Boost)

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

target_link_libraries(client PRIVATE
        pdh
        ws2_32
        Boost::beast
        Boost::asio
        Boost::system
        Boost::coroutine
        nlohmann_json::nlohmann_json
)

target_link_libraries(server PRIVATE
        Mswsock
        ws2_32
        wsock32
        Threads::Threads
        Boost::asio
        Boost::beast
        Boost::system
        Boost::coroutine
        Boost::thread
        nlohmann_json::nlohmann_json
)

set(MY_EXECUTABLES
        server
        client
)

foreach (MY_EXE ${MY_EXECUTABLES})
    set_target_properties(${MY_EXE} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
    )

    if (CMAKE_CONFIGURATION_TYPES) # For multi-config generators like Visual Studio, Xcode
        foreach (CONFIG ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER ${CONFIG} CONFIG_UPPER)
            set_target_properties(${MY_EXE} PROPERTIES
                    "RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER}" "${CMAKE_BINARY_DIR}"
            )
        endforeach ()
    else ()
    endif ()
endforeach ()
