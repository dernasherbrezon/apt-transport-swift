cmake_minimum_required(VERSION 2.8)
project(apt-transport-swift)

file(GLOB SOURCES src/*.c)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_library(swiftlib ${SOURCES})

include_directories(/usr/local/include)
include_directories(/opt/local/include)

link_directories(/opt/local/lib)
link_directories(/usr/local/lib)

add_executable(swift src/main.c)
target_link_libraries(swift swiftlib curl ssl crypto)

enable_testing()

file(GLOB TEST_SOURCES test/test_*.c)
file(GLOB TEST_RESOURCES test/resources/*)
file(COPY ${TEST_RESOURCES} DESTINATION "${CMAKE_BINARY_DIR}")

foreach(curTest ${TEST_SOURCES})
	get_filename_component(curTestName ${curTest} NAME_WE)
	add_test(NAME ${curTestName} COMMAND ${curTestName} ${curTest})
	add_executable(${curTestName} ${curTest})
	target_link_libraries(${curTestName} swiftlib curl ssl crypto check)
endforeach()
