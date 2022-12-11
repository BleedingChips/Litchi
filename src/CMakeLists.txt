cmake_minimum_required (VERSION 3.24)

add_library(
	LitchiLib
	STATIC
	LitchiHttpConnect.cpp
)

target_compile_features(LitchiLib PUBLIC cxx_std_23)
target_link_libraries(LitchiLib PUBLIC asio)
target_link_libraries(LitchiLib PUBLIC PotatoLib)
target_include_directories(LitchiLib PUBLIC "../include/")