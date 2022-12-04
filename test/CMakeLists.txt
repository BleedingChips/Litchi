cmake_minimum_required (VERSION 3.24)

add_executable(
	LitchiUnitTest
	UnitTest.cpp
)

target_compile_features(LitchiUnitTest PUBLIC cxx_std_23)
target_link_libraries(LitchiUnitTest PUBLIC asio)
target_link_libraries(LitchiUnitTest PUBLIC LitChiLib)