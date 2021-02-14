#include "pandaFramework.h"
#include "pandaSystem.h"

int main(int argc, char* argv[]) {
    // Open a new window framework and set the title
    PandaFramework framework;
    framework.open_framework(argc, argv);
    framework.set_window_title("My Panda3D Window");

    // Open the window
    WindowFramework* window = framework.open_window();
    NodePath camera = window->get_camera_group(); // Get the camera and store it

    framework.main_loop();
    framework.close_framework();
    return 0;
}