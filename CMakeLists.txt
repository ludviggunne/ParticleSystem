
cmake_minimum_required(VERSION 3.15)

project(ParticleSystem)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(extern/glfw)

add_executable(ParticleSystem src/main.cpp extern/glad/src/glad.c)

include_directories(
  extern/glfw/include
  extern/glm/
  extern/glad/include
)

target_link_libraries(ParticleSystem glfw ${GLFW_LIBRARIES})