cmake_minimum_required (VERSION 3.25.1)

ENABLE_CPP_MODULE()

add_library(
	LitchiModule 
	STATIC
	"LitchiContext.cpp"
	"LitchiSocketExecutor.cpp"
	"LitchiHttpConnect.cpp"
	"LitchiCompression.cpp"
)

target_sources(
	LitchiModule PUBLIC
	FILE_SET cxx_modules TYPE CXX_MODULES
	FILES
		"LitchiCompression.ixx"
		"LitchiContext.ixx"
		"LitchiMemory.ixx"
		"LitchiSocketConnect.ixx"
		"LitchiHttpConnect.ixx"
)


target_link_libraries(LitchiModule PUBLIC PotatoModule)
target_link_libraries(LitchiModule PRIVATE asio)

target_link_libraries(LitchiModule PRIVATE zlibstatic)
target_link_libraries(LitchiModule PRIVATE minizipstatic)