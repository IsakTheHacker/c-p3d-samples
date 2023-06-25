# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory.  No install
# targets were made, since this isn't a cmake tutorial.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

# The project name can be anything; it doesn't affect the results.
set(EXEC_NAME boxing-robots)
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
# panda3d subdirectory.  The HINT option allows it to at least try
# the same place first, but it's possible you'll have a lot of typing
# to do if cmake can't just find it.
find_library(PANDALIB panda PATH_SUFFIXES panda3d REQUIRED)
cmake_path(SET PANDALIB_DIR "${PANDALIB}")
cmake_path(GET PANDALIB_DIR PARENT_PATH PANDALIB_DIR)
set(PANDALIBS "${PANDALIB}")
foreach(x p3framework p3direct pandaexpress p3dtoolconfig p3dtool)
  find_library(PANDA-${x} ${x} HINT "${PANDALIB}" PATH_SUFFIXES panda3d REQUIRED)
  list(APPEND PANDALIBS "${PANDA-${x}}")
endforeach()

# Finally, in order to run these demos, I bake in the samples dir, if present.
find_file(SAMPLE_DIR_${EXEC_NAME} samples/${EXEC_NAME}/main.py 
          PATHS /usr/share /usr/local/share  PATH_SUFFIXES panda3d
          DOC "The original Python demo (for assets)")
if(SAMPLE_DIR_${EXEC_NAME})
  cmake_path(SET SAMPLE_DIR "${SAMPLE_DIR_${EXEC_NAME}}")
  cmake_path(GET SAMPLE_DIR PARENT_PATH SAMPLE_DIR)
  add_compile_definitions("SAMPLE_DIR=\"${SAMPLE_DIR}\"")
endif()

# And my own simple extensions to make porting easier
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../supt")

#### Build executable
set(SRC main.cpp)
add_executable(${EXEC_NAME} ${SRC} ../supt/anim_supt.cpp)
target_link_libraries(${EXEC_NAME} Eigen3::Eigen ${PANDALIBS})

install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/${EXEC_NAME}" TYPE BIN)
