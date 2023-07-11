# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

set(SAMPLE motion-trails)

# The project name can be anything; it doesn't affect the results.
project(${SAMPLE} LANGUAGES CXX)

include(../supt/samples.cmake)

#### Build executables
foreach(x framebuffer-feedback) # fireball) # fireball is too Python-heavy
  add_executable(${x} ${x}.cpp)
  target_link_libraries(${x} ${PANDALIBS} sample_supt)
  install(TARGETS ${x})
endforeach()
