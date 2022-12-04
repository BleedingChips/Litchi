cmake_minimum_required (VERSION 3.20)

project ("Litchi"  VERSION 1.24 LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
	fcasio
	URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-24-0.zip
	DOWNLOAD_EXTRACT_TIMESTAMP 600
)

FetchContent_MakeAvailable(fcasio)

add_library(asio INTERFACE)

target_include_directories(asio INTERFACE ${fcasio_SOURCE_DIR}/asio/include/)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory("test")
endif()