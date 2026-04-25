cmake_minimum_required(VERSION 3.10)
project(maze_runner C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find raylib (installed via brew install raylib)
find_package(raylib 5.0 QUIET)
if(NOT raylib_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(RAYLIB REQUIRED raylib)
endif()

add_executable(maze_runner
    src/main.c
    src/maze.c
    src/player.c
    src/game.c
    src/draw.c
)

if(RAYLIB_FOUND)
    target_link_libraries(maze_runner raylib)
else()
    target_link_libraries(maze_runner ${RAYLIB_LIBRARIES})
    target_include_directories(maze_runner PRIVATE ${RAYLIB_INCLUDE_DIRS})
endif()

# Enable debug symbols if needed
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g)
endif()
