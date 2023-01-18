cmake_minimum_required (VERSION 3.20)

add_library(
	LitchiLib 
	STATIC
	LitchiSocketConnect.cpp
 "LitchiHttpConnect.cpp" "LitchiCompression.cpp")

target_include_directories(LitchiLib PUBLIC "../include/")
target_link_libraries(LitchiLib PUBLIC asio)
target_link_libraries(LitchiLib PUBLIC PotatoLib)
target_link_libraries(LitchiLib PRIVATE zlibstatic)
target_link_libraries(LitchiLib PRIVATE minizipstatic)