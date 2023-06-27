# Build in-tree with "cmake . && cmake --build -j9".  Supports out-of-tree
# builds as well.  For example: "cmake -S . -B bin && cmake --build bin -j9"
# If cmake fails, use ccmake or cmake-gui (with -B if needed) to resolve.
# The resulting executable will be in the build directory.
# There is no need for 3.21.0; I picked that version pretty much at random.
cmake_minimum_required(VERSION 3.21.0)

set(SAMPLE solar-system)

# The project name can be anything; it doesn't affect the results.
project(${SAMPLE} LANGUAGES CXX)

include(../supt/samples.cmake)

foreach(x step1_blank_window step2_basic_setup step3_load_model
          step4_load_system)
  add_executable(${x} ${x}.cpp)
  target_link_libraries(${x} ${PANDALIBS})
  install(TARGETS ${x})
endforeach()
foreach(x step5_complete_solar_system step6_controllable_system)
  add_executable(${x} ${x}.cpp ${SUPT_SRC})
  target_link_libraries(${x} ${PANDALIBS} ${PANDA-p3direct})
  install(TARGETS ${x})
endforeach()
