# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory.  No install
# targets were made, since this isn't a cmake tutorial.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

# The project name can be anything; it doesn't affect the results.
set(EXEC_NAME ball-in-maze)
project(${EXEC_NAME} LANGUAGES CXX)

#### Find prerequisites
# Eigen3 is used optionally by Panda3D and necessarily by this code.
# It supports cmake, so find_package() should work.
# It sets Eigen3::Eigen for the library dependency.  This should also
# automatically set include dependencies, but this doesn't work, so I
# added it manually.
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLULDE_DIR})

# For Panda3D, I just search for panda.h.  All include files should be
# in the same place.  I let cmake search the standard places, plus a
# panda3d subdirectory.  If not found, you will be prompted for the file
# location in ccmake/cmake-gui.
find_file(PANDA_H panda.h PATH_SUFFIXES panda3d REQUIRED
          DOC "The main Panda3D include file (panda.h) in its installed home.")
cmake_path(SET PANDA_H "${PANDA_H}")
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
include_directories("${PANDA_H}")

# Even though the libraries are probably all together in one location,
# it's not as easy to just look in the same place.  Instead, I look for
# every required library in the standard places, again looking in a
# panda3d subdirectory.  Yes, this means if this code doesn't find it,
# you'll have a lot of typing to do to correct it.  Feel free to fix it
# and report a fix.
set(PANDALIB)
foreach(x panda pandaexpress p3dtoolconfig p3dtool p3framework)
  find_library(PANDA-${x} ${x} PATH_SUFFIXES panda3d REQUIRED)
  list(APPEND PANDALIB "${PANDA-${x}}")
endforeach()

#### Build executable
set(SRC main.cpp functions.cpp init.cpp globals.cpp)
list(TRANSFORM SRC PREPEND src/)
add_executable(${EXEC_NAME} ${SRC})
target_link_libraries(${EXEC_NAME} Eigen3::Eigen ${PANDALIB})
