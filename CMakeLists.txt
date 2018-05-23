cmake_minimum_required(VERSION 2.8)
project(apt-transport-swift)
file(GLOB SOURCES ./src/*.c)
include_directories(/usr/local/include)
include_directories(/opt/local/include)

link_directories(/opt/local/lib)
link_directories(/usr/local/lib)

add_executable(swift ${SOURCES})
target_link_libraries(swift curl ssl crypto)