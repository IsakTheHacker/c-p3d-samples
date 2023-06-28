# This file allows building all samples in one go.
# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory, one under
# each sample subdirectory.  No install targets were made, since this isn't
# a cmake tutorial.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

project(cpp-panda3d LANGUAGES CXX)

subdirs(
    asteroids
    ball-in-maze
    boxing-robots
    bump-mapping
    carousel
    chessboard
    culling
    disco-lights
    looking-and-gripping
    media-player
    music-box       # seems to work, but fails lock assertion on exit
    shader-terrain
    solar-system
)
