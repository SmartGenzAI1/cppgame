cmake_minimum_required(VERSION 3.16)
project(TankWar3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

add_executable(tankwar3d
    src/main.cpp
    src/game.cpp
)

target_include_directories(tankwar3d PRIVATE include)
target_link_libraries(tankwar3d PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
)

# Copy shaders to build directory
add_custom_command(TARGET tankwar3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:tankwar3d>/shaders
)
