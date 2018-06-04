cmake_minimum_required(VERSION 2.8)
project(apt-transport-swift)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_C_FLAGS "-std=c99")

file(GLOB SOURCES src/*.c)
list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")

add_executable(swift src/main.c)

add_library(swiftlib ${SOURCES})
target_link_libraries(swift swiftlib)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
target_link_libraries(swift ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

find_package(CURL REQUIRED)
include_directories(${CURL_INCLUDE_DIRS})
target_link_libraries(swift ${CURL_LIBRARIES})

find_package(JSON-C REQUIRED)
include_directories(${JSON-C_INCLUDE_DIRS})
target_link_libraries(swift ${JSON-C_LIBRARIES} uriparser)

find_package(UriParser REQUIRED)
include_directories(${URIPARSER_INCLUDE_DIR})
target_link_libraries(swift ${URIPARSER_LIBRARY})

set(VALGRIND_COMMAND_OPTIONS "--error-exitcode=1 -q --tool=memcheck --leak-check=yes --show-reachable=yes")
include (CTest)
enable_testing()

file(GLOB TEST_SOURCES test/test_*.c)
file(GLOB TEST_RESOURCES test/resources/*)
file(COPY ${TEST_RESOURCES} DESTINATION "${CMAKE_BINARY_DIR}")

find_package(Check REQUIRED)
include_directories(${CHECK_INCLUDE_DIR})

find_library(LIBRT rt)
find_package(Threads REQUIRED)

foreach(curTest ${TEST_SOURCES})
	get_filename_component(curTestName ${curTest} NAME_WE)
	add_test(NAME ${curTestName} COMMAND ${curTestName} ${curTest})
	add_executable(${curTestName} ${curTest})
	set_property(TARGET ${curTestName} PROPERTY C_STANDARD 99)
	if(LIBRT)
		target_link_libraries(${curTestName} rt)
	endif()
	target_link_libraries(${curTestName} swiftlib ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${JSON-C_LIBRARIES} ${URIPARSER_LIBRARY} ${CHECK_LIBRARIES})
endforeach()

file(GLOB MANUAL_TEST_SOURCES test/manual_*.c)
foreach(curTest ${MANUAL_TEST_SOURCES})
	get_filename_component(curTestName ${curTest} NAME_WE)
	add_executable(${curTestName} ${curTest})
	set_property(TARGET ${curTestName} PROPERTY C_STANDARD 99)
	if(LIBRT)
		target_link_libraries(${curTestName} rt)
	endif()
	target_link_libraries(${curTestName} swiftlib ${CMAKE_THREAD_LIBS_INIT} ${CURL_LIBRARIES} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY} ${JSON-C_LIBRARIES} ${URIPARSER_LIBRARY})
endforeach()
