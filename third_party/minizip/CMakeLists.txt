cmake_minimum_required (VERSION 3.20)

project (minizip VERSION 1.2.13 LANGUAGES C)

add_library(
	minizipcomplie 
	STATIC
	src/ioapi.c
	src/iowin32.c
	src/miniunz.c
	src/mztools.c
	src/unzip.c
	src/zip.c
)

target_include_directories(minizipcomplie PRIVATE "include/minizip")
target_link_libraries(minizipcomplie PUBLIC zlibcomplie)

add_library(minizipstatic INTERFACE)

target_include_directories(minizipstatic INTERFACE "include/")
target_link_libraries(minizipstatic INTERFACE minizipcomplie)
target_link_libraries(minizipstatic INTERFACE zlibstatic)