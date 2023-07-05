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

# Finally, in order to run these demos, I bake in the samples dir, if present.
find_file(SAMPLE_DIR samples/media-player/PandaSneezes.ogv
          PATHS /usr/share /usr/local/share  PATH_SUFFIXES panda3d
          DOC "The original Python samples (for assets)")
if(SAMPLE_DIR)
  cmake_path(SET SAMPLE_DIR "${SAMPLE_DIR}")
  cmake_path(GET SAMPLE_DIR PARENT_PATH SAMPLE_DIR)
  cmake_path(GET SAMPLE_DIR PARENT_PATH SAMPLE_DIR)
  add_compile_definitions("SAMPLE_DIR=\"${SAMPLE_DIR}/${SAMPLE}\"")
endif()

# And my own simple extensions to make porting easier
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../supt")
set(SUPT_SRC ../supt/anim_supt.cpp)
