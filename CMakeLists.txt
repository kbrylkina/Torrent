cmake_minimum_required(VERSION 3.16)
project(torrent-file CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

include(FetchContent)

set(CPR_USE_SYSTEM_CURL ON)
FetchContent_Declare(
    cpr
    GIT_REPOSITORY https://github.com/libcpr/cpr.git
    GIT_TAG 1.11.2
)
FetchContent_MakeAvailable(cpr)

add_executable(
    ${PROJECT_NAME}
    main.cpp
        check_peers.h
        peer.h
        torrent_file.h
        peer_connect.cpp
        peer_connect.h
        tcp_connect.cpp
        tcp_connect.h
        torrent_tracker.cpp
        torrent_tracker.h
        torrent_file.cpp
        message.cpp
        message.h
        byte_tools.h
        byte_tools.cpp
        piece_storage.cpp
        piece_storage.h
        piece.cpp
        piece.h
)
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES} cpr::cpr)