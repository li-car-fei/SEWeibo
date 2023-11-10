cmake_minimum_required(VERSION 3.5)
project(server)

include_directories(
    ${PROJECT_SOURCE_DIR}/cppjieba/include
    ${PROJECT_SOURCE_DIR}/limonp/include
    ${PROJECT_SOURCE_DIR}/jsoncpp/include
    ${PROJECT_SOURCE_DIR}/log
    ${PROJECT_SOURCE_DIR}/redis
)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -std=c++11")

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(NET_DIR ${CMAKE_CURRENT_SOURCE_DIR}/net)
set(THREADPOOL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/threadpool)
set(CACHE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/redis)
set(LOG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/log)

file(GLOB SRCS ${SRC_DIR}/*.cpp ${NET_DIR}/*.cpp ${THREADPOOL_DIR}/*.cpp ${CACHE_DIR}/*.cpp ${LOG_DIR}/*.cpp)

add_executable(server ${SRCS})

# 添加redis相关依赖
find_path(HIREDIS_HEADER hiredis)
find_library(HIREDIS_LIB hiredis)
find_path(REDIS_PLUS_PLUS_HEADER sw)
find_library(REDIS_PLUS_PLUS_LIB redis++)

target_include_directories(server
    PUBLIC ${HIREDIS_HEADER}
    PUBLIC ${REDIS_PLUS_PLUS_HEADER}
)
target_link_libraries(server
    ${HIREDIS_LIB}
    ${REDIS_PLUS_PLUS_LIB}
)

target_link_libraries(server
    pthread
    log4cpp
    ${PROJECT_SOURCE_DIR}/jsoncpp/build/lib/libjsoncpp.a
)

add_custom_target(cleanall
    COMMAND ${CMAKE_COMMAND} -E remove server
)

# add_dependencies(cleanall clean)