cmake_minimum_required (VERSION 3.25.1)

project (AsioWrapper VERSION 1.0 LANGUAGES CXX)

add_library(
	AsioWrapper 
	STATIC
	"src/AsioWrapper.cpp"
)

target_include_directories(AsioWrapper PUBLIC "include/")
target_link_libraries(AsioWrapper PRIVATE asio)