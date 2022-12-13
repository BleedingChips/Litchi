cmake_minimum_required (VERSION 3.24)

add_library(
	Litchi
	STATIC
	LitchiHttpConnect.cpp
	LitchiSocketConnect.cpp
)

target_compile_features(Litchi PUBLIC cxx_std_23)
target_link_libraries(Litchi PUBLIC asio)
target_link_libraries(Litchi PUBLIC Potato)
target_include_directories(Litchi PUBLIC "../include/")