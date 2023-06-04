cmake_minimum_required (VERSION 3.26)

ENABLE_CPP_MODULE()

add_executable(
	Http11Test
	"Http11Test.cpp"
)



target_link_libraries(Http11Test PUBLIC LitchiModule)