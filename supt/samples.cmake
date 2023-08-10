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
  cmake_path(SET SDIR "${SAMPLE_DIR}")
  cmake_path(GET SDIR PARENT_PATH SDIR)
  cmake_path(GET SDIR PARENT_PATH SDIR)
  if(EXISTS ${SDIR}/${SAMPLE}) # global build includes w/ invalid name for lib
    add_compile_definitions("SAMPLE_DIR=\"${SDIR}/${SAMPLE}\"")
  endif()
endif()

# precompile headers
function(precompile_headers)
  if(${ARGC} EQUAL 0)
    set(ARGN ${SAMPLE})
  endif()
  foreach(t ${ARGN})
    if(NOT PRECOMPILE_ROOT)
      set(PRECOMPILE_ROOT "${t}" CACHE STRING "DO NOT TOUCH")
      mark_as_advanced(PRECOMPILE_ROOT)
    endif()
    if("${PRECOMPILE_ROOT}" STREQUAL "${t}")
      set(sample_supt supt/sample_supt.h)
      while(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${sample_supt}")
        set(sample_supt "../${sample_supt}")
      endwhile()
      target_precompile_headers(${t} PUBLIC "${PANDA_H}/panda3d/pandaFramework.h"
                                ${sample_supt})
    else()
      target_precompile_headers("${t}" REUSE_FROM ${PRECOMPILE_ROOT})
    endif()
  endforeach()
endfunction()

# And my own simple extensions to make porting easier
if(NOT SUPT_SRC) # only do this once if building everything
  set(SUPT_SRC sample anim interval sound particle)
  list(TRANSFORM SUPT_SRC PREPEND ../supt/)
  list(TRANSFORM SUPT_SRC APPEND _supt.cpp)
  add_library(sample_supt EXCLUDE_FROM_ALL ${SUPT_SRC}) # only build if needed
  precompile_headers(sample_supt)
  target_link_libraries(sample_supt ${PANDALIBS})
endif()
if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/supt/samples_supt.h)
  include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../supt")
endif()
