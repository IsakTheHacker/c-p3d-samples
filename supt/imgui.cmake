# Include fie for doing panda3d_imgui
# Builds imgui and panda3d_imgui from sources
# Some extra stuff borrowed from another project (PD)

set(IMGUI_SRC imgui imgui_draw imgui_widgets imgui_tables
              misc/cpp/imgui_stdlib imgui_demo)
set(IMGUI_INCLUDE "" misc/cpp)
set(IMGUI_LIBS)
# Fonts (I need big fonts because I'm going blind)
set(USE_FT2 YES CACHE BOOL "Use Freetype2 for font rendering")
if(USE_FT2)
  include(FindPkgConfig)
  pkg_check_modules(FT2 freetype2 REQUIRED)
  add_compile_definitions(IMGUI_ENABLE_FREETYPE)
  add_compile_options(${FT2_CFLAGS})
  list(APPEND IMGUI_LIBS ${FT2_LIBRARIES})
  list(APPEND IMGUI_SRC misc/freetype/imgui_freetype)
  list(APPEND IMGUI_INCLUDE misc/freetype)
endif()
list(TRANSFORM IMGUI_SRC PREPEND ../supt/imgui/)
list(TRANSFORM IMGUI_INCLUDE PREPEND ../supt/imgui/)

list(APPEND IMGUI_SRC ../supt/panda3d_imgui/panda3d_imgui/panda3d_imgui)
list(APPEND IMGUI_INCLUDE ../supt/panda3d_imgui/panda3d_imgui)

list(APPEND IMGUI_SRC ../supt/imgui_supt)

list(TRANSFORM IMGUI_INCLUDE PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
include_directories(${IMGUI_INCLUDE})

foreach(x frag vert)
  set(shname ${CMAKE_CURRENT_SOURCE_DIR}/../supt/panda3d_imgui/panda3d_imgui/shader/panda3d_imgui.${x}.glsl)
  # cmake's list parsing is totally broken
  # so I just process it all as one string
  file(READ "${shname}" shader)
  string(REPLACE "\r" "" shader "${shader}")
  string(REGEX REPLACE "[\\\\\"]" [[\\\0]] shader "${shader}")
  string(REPLACE "\n" "\\n\"\n\"" shader "${shader}")
  set(shader_${x} "\"${shader}\"")
endforeach()
configure_file(../supt/panda3d_imgui_shaders.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/panda3d_imgui_shaders.h)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# use fontconfig to avoid having to remember file names
include(FindPkgConfig)
pkg_check_modules(FC fontconfig REQUIRED)
add_compile_options(${FC_CFLAGS})
list(APPEND IMGUI_LIBS ${FC_LIBRARIES})

# file selector widget
# [note: find_local_file is part of the other project; replace it]
# FIXME: test-compile to find version #
# Unfortunately, it stopped updating tags, so I'm not sure it's accurate
#find_local_file(FILE_DIALOG_H ImGuiFileDialog.h "ImGuiFileDialog header"
#                imgui ImGuiFileDialog ImGuiFileDialog/ImGuiFileDialog)
#find_local_file(FILE_DIALOG_CPP ImGuiFileDialog.cpp
#                "ImGuiFileDialog source; if blank: assumed in main library"
#                imgui ImGuiFileDialog ImGuiFileDialog/ImGuiFileDialog)
#if(NOT FILE_DIALOG_H)
#  message(SEND_ERROR
#   "ImGuiFileDialog.h not found.\n"
#   "See INSTALL.md or just \"make external\".")
#endif()

## Markdown widget
## Version?
## Using 61a181bdb83f450f852f7cf5d1282d8cda1c0f57
#find_local_file(IMGUI_MD_H imgui_markdown.h "imgui_markdown header"
#                imgui imgui_markdown)
#if(NOT IMGUI_MD_H)
#  message(SEND_ERROR
#  "imgui_markdown.h not found.\n"
#  "See INSTALL.md or just \"make external\".")
#endif()

list(TRANSFORM IMGUI_SRC APPEND .cpp)
add_library(panda3d_imgui ${IMGUI_SRC})
target_link_libraries(panda3d_imgui Eigen3::Eigen)
# I prefer <panda3d/...>, but panda3d_imgui uses <...>
target_include_directories(panda3d_imgui PUBLIC ${PANDA_H}/panda3d)
list(PREPEND IMGUI_LIBS panda3d_imgui)
