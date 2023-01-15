cmake_minimum_required (VERSION 3.20)

project (zlib VERSION 1.2.13 LANGUAGES C)

add_library(
	zlibcomplie 
	STATIC
	src/adler32.c
	src/compress.c
	src/crc32.c
	src/deflate.c
	src/gzclose.c
	src/gzlib.c
	src/gzread.c
	src/gzwrite.c
	src/infback.c
	src/inffast.c
	src/inflate.c
	src/trees.c
	src/uncompr.c
	src/zutil.c
)

target_include_directories(zlibcomplie PUBLIC "include/zlib")

add_library(zlibstatic INTERFACE)

target_include_directories(zlibstatic INTERFACE "include/")
target_link_libraries(zlibstatic INTERFACE zlibcomplie)