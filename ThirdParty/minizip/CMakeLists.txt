cmake_minimum_required (VERSION 3.20)

project (minizip VERSION 1.2.13 LANGUAGES C)

add_library(
	minizipstatic 
	STATIC
	src/ioapi.c
	src/iowin32.c
	src/miniunz.c
	src/mztools.c
	src/unzip.c
	src/zip.c
)

target_include_directories(minizipstatic PRIVATE "include/minizip")
target_link_libraries(minizipstatic PUBLIC zlibstatic)