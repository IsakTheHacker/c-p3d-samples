# This file allows building all reference code in one go.
# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory, one under
# each sample subdirectory.  No install targets were made, since this isn't
# a cmake tutorial.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

project(Panda3D-tutorial LANGUAGES CXX)

set(i 1)
while(${i} LESS_EQUAL 16)
   add_subdirectory(Lesson${i})
   math(EXPR i "${i}+1")
endwhile()
