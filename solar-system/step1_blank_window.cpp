/*
 *  Translation of Python solar-system sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/solar-system
 *
 * Original C++ conversion by Thomas J. Moore June 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Shao Zhang and Phil Saltzman
 * Last Updated: 2015-03-13
 *
 * This tutorial is intended as an initial panda programming lesson going over
 * display initialization, loading models, placing objects, and the scene graph.
 *
 * Step 1: PandaFramework contains the main Panda3D modules. Initilize using
 * its open_framework() method.  The window is then created using its
 * open_window() method.  The main_loop() method causes the real-time
 * simulation to begin.  When finished, you can also clean up using
 * close_framework().
 *
 * Compiling and linking is beyond the scope of this document.  See
 * CMakeLists.txt for a sample cross-platform build control.  In
 * general, you will have to include the panda3d installed header
 * directory in your include path (e.g. on the Debian default installer,
 * -I/usr/include/panda3d) when compiling objects.  Link with the
 * installed libraries, whose location and names vary wildly.   In
 * generaly, you will at least want the framework library and its
 * dependencies: p3dframework panda pandaexpress p3dtoolconfig
 * p3dtool.  Many of the samples also use p3direct.  While Panda3D
 * uses many other libraries, the only external library that appears
 * to have to be manually included is Eigen (Eigen3), if enabled during
 * the SDK build.
 *
 * Unlike the Python ShowBase, no default event handlers are given.  As such,
 * this also shows how to set up the default keyboard and mouse controls,
 * so that at least ESC can be used to exit.
 */

// The main header file should be in your include search path.
// I prefer <> over "" to indicate "system" header files, but either
// should work if you set the include search path correctly.
#include <panda3d/pandaFramework.h>

// These are stored globally, but of course that's not necessary or
// even a good idea in large projects.
PandaFramework framework;
WindowFramework *window;

int main(void)
{
    // init
    framework.open_framework();
    window = framework.open_window();
    window->setup_trackball();
    window->enable_keyboard();
    framework.enable_default_keys();

    // run
    framework.main_loop();

    // cleanup
    framework.close_framework();
}
