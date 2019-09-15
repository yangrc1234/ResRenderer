cmake_minimum_required(VERSION 3.2)
PROJECT(ResRenderer)
set (CMAKE_CXX_STANDARD 11)

#GLEW
file(GLOB_RECURSE GLEW_SOURCES 3rdparty/glew/src/*.c)
add_library(GLEW ${GLEW_SOURCES})
target_compile_definitions(GLEW PUBLIC -D GLEW_STATIC)
target_include_directories(GLEW PRIVATE 3rdparty/glew/include)
set_property(TARGET GLEW PROPERTY POSITION_INDEPENDENT_CODE ON)

#GLFW
SET (GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
SET (GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
SET (GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
SET (GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(3rdparty/glfw)

#OPENGL
find_package(OpenGL REQUIRED)

#ResRenderer sources
file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp include/*.hpp)
add_library(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} 
  PRIVATE 3rdparty/glew/include
  PRIVATE 3rdparty/glfw/include
  PRIVATE src/
  PUBLIC include/
  PUBLIC ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(${PROJECT_NAME} 
  PUBLIC ${OPENGL_gl_LIBRARY}
  PRIVATE glfw
  PRIVATE GLEW
  )
target_compile_options(${PROJECT_NAME} PRIVATE -DOPENGL -DRES_RENDERER_BUILD)

#Sample
add_executable(Sample sample/Application.cpp)
target_link_libraries(Sample PRIVATE ${PROJECT_NAME})
target_include_directories(Sample PRIVATE include/)

#Other stuff.
if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -pedantic -Werror)
endif()