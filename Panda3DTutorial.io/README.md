# Panda3DTutorial
A beginner's tutorial for Panda3D, using C++.

See https://arsthaumaturgis.github.io/Panda3DTutorial.io/ for the
original tutorial, in Python.  It is the final tutorial I would
consider semi-official, since it is linked to from the User Manual's
last page (even the C++ one, in spite of the fact that the tutorial
doesn't even mention C++).

I was going to duplicate the web site materials as wel, but decided
against it.   There is nothing wrong with just reading the original
tutorial, as written, and then translating the contents to C++ as
needed.

As such, this project's goals, and this documentation's goals, are
somewhat different.  The following expects you to first read:

[Start Here](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/prologue.html)

What it is:

* __Collected notes on differences between development for Panda3D in
Python and C++__.  Most of the documentation and samples you'll find
out there are written only for Python.  You can use this as a partial
translation dictionary.  This includes functional and similarly
structured translations of all of the code provided in the original
tutorial.  Just like the original, this will result in a fully
functional game with full feature parity with the original.

What it is not:

* __A C++ tutorial.__ This tutorial assumes a basic knowledge of
C++-11.  You will also be expected to know how to turn C++ code into
executables on your platform, which also includes a basica knowledge
of linker libraries and locations, header files and locations, and so
forth.  You don't have to be an expert, but if you're entirely new to
the language then I recommend picking up a C++ tutorial before taking
on this one!

* __A C++ advocacy document.__  If you like Python, use it.  If you
don't understand why anyone would use C++, don't worry about it.  If
making your game faster is the only reason you want to try C++, you
will likely be disappointed, since your game will likely spend most of
its CPU time in-engine, making any performance improvements due to
your application being in C++ far less critical.  In fact, I would
recommend resorting to Python accelarators like PyPy before switching
your entire code base and preferred programming language to C++.
There are many other differences between Python and C++, some of which
may aggravate you, and some of which may please you.  That, and that
alone should determine which language to use. The lack of samples and
documentation should not be a factor, but unfortunately, with Panda3D,
it is.

* __A standalone tutorial.__  As mentioned above, my original plan was
to do exactly that, but I backed off due to lack of time.  Maybe a
future version will be more standalone.  You will be expected to read
the Python tutorial first.  If you don't like Python, hold your nose.
If you can't understand the Python code (although, really, most
imperative languages should be understandable by anyone with basic
competeance in at least one such language), then just ignore the code
samples and look here.

* __A Build system tutorial.__ This tutorial's code was built using
the CMake cross-platform build tool.  It should be possible to build
on common platforms without issue; if you find one, feel free to let
me know.  You will have to check the SDK installation documentation
for your platform to find out how to get to the libraries and include
files, as well as how to compile code into executables if you want to
use any other method.  In particular, the common practice on most
non-UNIX environments is to encode all the build instrucitons into the
IDE.  CMake can do this for you for some IDEs, so you can use that as
a starting point if you like.

What's covered:
---------------

* The fundamentals of Panda3D
* Lights and automatic shading
* Input-handling
* Tasks and "update" methods
* Simple collision-detection
* Music and sound effects
* GUI construction (not yet ready in C++)
* Building a distributable version (not yet ready in C++)
* And more besides!

This repository is primarily built around the reference code, so you
will find the code for each lesson next to its document, in the
numbered Lesson subdirectory.

See the Python prologue document for information about what assets are
needed, and where to put them.

Building with CMake
-------------------

Now, a longer aside, that should probably go into Lesson 1, but it
clutters up the lesson too much.  As mentioned, this is not a build
system tutorial, but you do, in the end, have to build the C++ code
into executables to run.  Here's how to do it with CMake, at least on
UNIX (and probably on others as well; as mentioned above, if you have
issues on other platforms, let me know).

To control CMake, you need to create a `CMakeLists.txt` file.
First, there's boilerplate at the top.  There's always boilerplate,
isn't there?

```cmake
# My choice of 3.21.0 is pretty arbitrary.
cmake_minimum_required(VERSION 3.21.0)

project(Lesson1 LANGUAGES CXX)
```

Then, we need to find the SDK.  On Linux, at least as of Panda3D
1.10.13, the include files and libraries are placed in a `panda3d`
subdirectory.  For include files, this is a good idea, as mentioned
above, so all the sample code here will use `<panda3d/..>` to load
include files.  This means that if your SDK doesn't place them in a
panda3d subdirectory, you'll just have to make one.  I think that is a
better long-term answer than fixing my code.

In CMake, rather than having to know how to do things in an
OS-specific and compler-specific manner, you have to know how to do it
in a cmake-specific manner:

```cmake
# Find <panda3d/panda.h> in common locations
# If it fails, you can set the full path of panda.h using
# -DPANDA_H=<path> or using ccmake or cmake-gui.
find_file(PANDA_H panda.h PATH_SUFFIXES panda3d REQUIRED
          DOC "The main Panda3D include file (panda3d/panda.h) in its installed home.")
cmake_path(SET PANDA_H "${PANDA_H}")
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
cmake_path(GET PANDA_H PARENT_PATH PANDA_H)
include_directories("${PANDA_H}")
```

For libraries, it is actually a problem.  The full path to the
libraries must be added to your library path, so that you can specify
libraries by their simple naame.

```cmake
# Find the base panda library.  Once again, if it cant find it,
# give the full path in PANDALIB.
find_library(PANDALIB panda PATH_SUFFIXES panda3d REQUIRED)
# Find the rest of the libraries, starting in the same directory
# as PANDALIB.  Accumulate all libraries into PANDALIBS.
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
# Eigen3 is almost required for vector math, even though it's
# techincally optional.
# It supports cmake, so find_package() should work.
# It sets Eigen3::Eigen for the library dependency.  This should also
# automatically set include dependencies, but this doesn't work, so I
# added it manually.
find_package(Eigen3 REQUIRED)
include_directories(${Eigen3_INCLULDE_DIR})
list(APPEND PANDALIBS Eigen3::Eigen)
# pthreads is required on some UNIX systems.
find_package(Threads)
if(CMAKE_THREAD_LIBS_INIT)
  list(APPEND PANDALIBS "${CMAKE_THREAD_LIBS_INIT}")
endif()
```

Right, now all that's left is to actually build an executable.  Maybe
install it, as well.  Since I'm using `${PROJECT_NAME}` as the target,
the resulting executable's name will be the same as the project name
(with a `.exe` extension on Windows).

```cmake
add_executable(${PROJECT_NAME} Game.cpp)
target_link_libraries(${PROJECT_NAME} ${PANDALIBS})
install(TARGETS ${PROJECT_NAME})
```

Note that this is a standalone `CMakeLists.txt`, and is present in
every lesson subdictory.  You can build a single lesson in-tree with:

```
cmake .
cmake --build . -j9
```

or out-of-tree (e.g. in a "build" directory) with:

```
cmake -S . -B build
cmake --build build -j9`
```

If either command fails, you might be able to figure out how to fix it
using `ccmake` or `cmake-gui`.

I have also provided a top-level `CMakeLists.txt` which simply includes all
subdirectories, so you can build them all at once with a single command:

```cmake
cmake_minimum_required(VERSION 3.21.0)

project(Panda3D-tutorial LANGUAGES CXX)

set(i 1)
while(${i} LESS_EQUAL 16)
   add_subdirectory(Lesson${i})
   math(EXPR i "${i}+1")
endwhile()
```

Proceed to Lesson1 to continue.

Sorry for not providing a link, as it is hard to link to other md
files in anything resembling a portable manner.  Even if I stick to
github.
