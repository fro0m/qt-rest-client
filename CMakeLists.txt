cmake_minimum_required(VERSION 3.15)
project(qt-rest-client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network)

add_library(qt-rest-client STATIC
    HttpRequestWorker.cpp
    HttpRequestWorker.h
)

target_link_libraries(qt-rest-client
    PUBLIC
        Qt6::Core
        Qt6::Network
)

target_include_directories(qt-rest-client
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

install(TARGETS qt-rest-client
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(FILES HttpRequestWorker.h
    DESTINATION include
)