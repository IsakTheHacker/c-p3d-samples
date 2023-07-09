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

# create supt library only once, globally
set(SAMPLE none)
include(supt/samples.cmake)

subdirs(
    asteroids
    ball-in-maze
    boxing-robots
    bump-mapping
    carousel
    cartoon-shader  # only advanced works, and no BufferViewer
    chessboard
    culling
    disco-lights
    distortion      # no BufferViewer
    fireflies       # no BufferViewer; sometimes crashes on model load
    fractal-plants
    gamepad         # no GUI-heavy sample yet
    glow-filter     # only advanced works, and no BufferViewer
    infinite-tunnel
    looking-and-gripping
    media-player
#    motion-trails # not yet
    mouse-modes
    music-box       # fails lock assertion on exit
    procedural-cube
#    render-to-texture # not yet
    roaming-ralph
    shader-terrain
    shadows         # no BufferViewer
    solar-system
#    particles      # not yet; probably not ever (too much Python)
#    rocket-console # not yet; probably never
)
