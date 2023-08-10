cmake_minimum_required(VERSION 3.21.0)

project(Lesson7 LANGUAGES CXX)

# from samples.cmake, since I want this to be standalone:
#### Find prerequisites
# For Panda3D, I just search for panada3d/panda.h.  All include files should
# be in the same place.  I let cmake search the standard places.  Note that
# this means that you have to install the inculde files in a panda3d
# subdirectory.  This is standard practice.
# If not found, you will be prompted for the file location in ccmake/cmake-gui.
find_file(PANDA_H panda.h PATH_SUFFIXES panda3d REQUIRED
          DOC "The main Panda3D include file (panda3d/panda.h) in its installed home.")
cmake_path(SET PANDA_H "${PANDA_H}")
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
include_directories("${PANDA_H}")

# Even though the libraries are probably all together in one location,
# it's not as easy to just look in the same place.  Instead, I look for
# every required library in the standard places, again looking in a
# panda3d subdirectory.  The HINT option allows it to at least try
# the same place first, but it's possible you'll have a lot of typing
# to do if cmake can't just find it.
find_library(PANDALIB panda PATH_SUFFIXES panda3d REQUIRED)
cmake_path(SET PANDALIB_DIR "${PANDALIB}")
cmake_path(GET PANDALIB_DIR PARENT_PATH PANDALIB_DIR)
set(PANDALIBS "${PANDALIB}")
foreach(x p3framework pandaexpress p3dtoolconfig p3dtool)
  find_library(PANDA-${x} ${x} HINT "${PANDALIB}" PATH_SUFFIXES panda3d REQUIRED)
  list(APPEND PANDALIBS "${PANDA-${x}}")
endforeach()
# Optional libraries
foreach(x p3direct)
  find_library(PANDA-${x} ${x} HINT "${PANDALIB}" PATH_SUFFIXES panda3d)
endforeach()
# Eigen3 is used optionally by Panda3D; some samples depend on it, though
# It supports cmake, so find_package() should work.
# It sets Eigen3::Eigen for the library dependency.  This should also
# automatically set include dependencies, but this doesn't work, so I
# added it manually.
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLULDE_DIR})
list(APPEND PANDALIBS Eigen3::Eigen)
# pthreads is required.  My gcc config adds -pthread by default, but some
# don't.
find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PANDALIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()

#### Build executable
add_executable(${PROJECT_NAME} Game.cpp GameObject.cpp)
precompile_headers(${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${PANDALIBS})
install(TARGETS ${PROJECT_NAME})
