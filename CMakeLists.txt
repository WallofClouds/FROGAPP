cmake_minimum_required(VERSION 3.12)
project(FROGAPP)
add_subdirectory(engine)

add_executable(FROGAPP
	"main.cpp"
	"embedded.hpp"
	"build/app.rc"
)
target_link_libraries(FROGAPP
	firesteel
	glad
	imgui
	glm
	assimp
)
target_compile_features(FROGAPP PUBLIC cxx_std_17)
target_include_directories(FROGAPP PUBLIC includes)
target_include_directories(FROGAPP PUBLIC engine/external/glfw)
set_target_properties(FROGAPP PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FROGAPP)