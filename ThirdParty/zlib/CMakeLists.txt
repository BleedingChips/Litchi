cmake_minimum_required (VERSION 3.20)

project (zlib VERSION 1.2.13 LANGUAGES C)

add_library(
	zlibstatic 
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
	src/inftrees.c
	src/trees.c
	src/uncompr.c
	src/zutil.c
)

target_include_directories(zlibstatic PUBLIC "include/zlib")
target_compile_definitions(zlibstatic PUBLIC ZLIB_CONST)