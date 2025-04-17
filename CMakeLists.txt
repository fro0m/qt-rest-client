# Qt REST Client CMakeLists.txt

add_library(qt_rest_client OBJECT)

target_sources(qt_rest_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
)

target_include_directories(qt_rest_client PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(qt_rest_client
    PRIVATE
    Qt6::Core
    Qt6::Network)