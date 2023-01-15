cmake_minimum_required (VERSION 3.20)

project ("Litchi"  VERSION 1.24 LANGUAGES CXX)

include(FetchContent)

if (NOT DEFINED asio_POPULATED)

FetchContent_Declare(
	asio
	URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-24-0.zip
	DOWNLOAD_EXTRACT_TIMESTAMP 600
)

FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include/)

target_compile_definitions(asio INTERFACE ASIO_NO_DEPRECATED)

message(STATUS "Use https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-24-0.zip As Asio Implement")

endif()

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory("../Potato" "Potato")
endif()

add_subdirectory("src")
add_subdirectory("third_party/zlib")
add_subdirectory("third_party/minizip")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory("test")
endif()