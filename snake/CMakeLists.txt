cmake_minimum_required(VERSION 3.21.0)

project(snake LANGUAGES CXX)

find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLULDE_DIR})
find_file(PANDA_H panda.h PATH_SUFFIXES panda3d REQUIRED)
cmake_path(SET PANDA_H ${PANDA_H})
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
include_directories(${PANDA_H})
set(PANDALIB)
foreach(x panda pandaexpress p3dtoolconfig p3dtool p3framework)
  find_library(PANDA-${x} ${x} PATH_SUFFIXES panda3d REQUIRED)
  list(APPEND PANDALIB "${PANDA-${x}}")
endforeach()

add_executable(snake src/main.cpp src/snake.cpp src/tasks.cpp)
target_link_libraries(snake Eigen3::Eigen ${PANDALIB})
