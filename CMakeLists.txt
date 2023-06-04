cmake_minimum_required (VERSION 3.26)

project ("Litchi"  VERSION 1.24 LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory("../Potato" "Potato")
endif()

ENABLE_CMAKE_CPP_MODULE()

ENABLE_CPP_MODULE()

#[[

include(FetchContent)

if (NOT DEFINED asio_POPULATED)

	FetchContent_Declare(
		asio
		URL https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-0.zip
		DOWNLOAD_EXTRACT_TIMESTAMP 600
	)

	FetchContent_MakeAvailable(asio)

	add_library(asio STATIC 
		ThirdParty/asio/src/asio.cpp
		#${asio_SOURCE_DIR}/asio/src/asio_ssl.cpp
	)

	set_target_properties(asio PROPERTIES LINKER_LANGUAGE CXX)


	target_include_directories(asio PUBLIC ${asio_SOURCE_DIR}/asio/include/)
	target_compile_definitions(asio PUBLIC ASIO_NO_DEPRECATED)
	target_compile_definitions(asio PUBLIC ASIO_SEPARATE_COMPILATION)
	target_compile_definitions(asio PUBLIC _WIN32_WINDOWS)

	#message(STATUS "Use https://github.com/chriskohlhoff/asio/archive/refs/tags/asio-1-28-0.zip As Asio Implement")

endif()
]]

add_library(asio STATIC 
	ThirdParty/asio/src/asio.cpp
	#${asio_SOURCE_DIR}/asio/src/asio_ssl.cpp
)

target_compile_definitions(asio PUBLIC ASIO_NO_DEPRECATED)
target_compile_definitions(asio PUBLIC ASIO_SEPARATE_COMPILATION)
target_compile_definitions(asio PUBLIC _WIN32_WINDOWS)
target_include_directories(asio PUBLIC ThirdParty/asio/include/)

add_subdirectory("ThirdParty/zlib")
add_subdirectory("ThirdParty/minizip")
#add_subdirectory("ThirdParty/AsioWrapper")
add_subdirectory("Litchi")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	add_subdirectory("test")
endif()