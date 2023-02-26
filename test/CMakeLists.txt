cmake_minimum_required (VERSION 3.25.1)

add_executable(
	Http11Test
	Http11Test.cpp
)

ENABLE_CPP_MODULE()

target_link_libraries(Http11Test PUBLIC LitchiModule)