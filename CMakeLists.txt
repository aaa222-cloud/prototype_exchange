cmake_minimum_required(VERSION 3.0.0)
project(exchange VERSION 0.1.0)

include(CTest)
enable_testing()

set(All_SRCS
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/market_data_publisher.hpp
    ${PROJECT_SOURCE_DIR}/market_data_publisher.cpp
    ${PROJECT_SOURCE_DIR}/matching_engine.hpp
    ${PROJECT_SOURCE_DIR}/matching_engine.cpp
    ${PROJECT_SOURCE_DIR}/event.hpp
    ${PROJECT_SOURCE_DIR}/event.cpp
    ${PROJECT_SOURCE_DIR}/exchange.hpp
    ${PROJECT_SOURCE_DIR}/exchange.cpp
    ${PROJECT_SOURCE_DIR}/order.hpp
    ${PROJECT_SOURCE_DIR}/order.cpp
    ${PROJECT_SOURCE_DIR}/order_book.hpp
    ${PROJECT_SOURCE_DIR}/order_book.cpp
    ${PROJECT_SOURCE_DIR}/price4.hpp 
    ${PROJECT_SOURCE_DIR}/price4.cpp
    ${PROJECT_SOURCE_DIR}/serialise.hpp
    ${PROJECT_SOURCE_DIR}/size_rules.hpp 
    ${PROJECT_SOURCE_DIR}/size_rules.cpp
    ${PROJECT_SOURCE_DIR}/stock.hpp 
    ${PROJECT_SOURCE_DIR}/stock.cpp
    ${PROJECT_SOURCE_DIR}/ticker_rules.hpp
    ${PROJECT_SOURCE_DIR}/ticker_rules.cpp
)
add_executable(exchange ${All_SRCS})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include(FetchContent)

FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/ArthurSonzogni/nlohmann_json_cmake_fetchcontent
    GIT_TAG v3.10.5)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE nlohmann_json::nlohmann_json)