#include "header/snake.h"
#include "load_prc_file.h"

std::string samplePath = "./";

int main(int argc, char* argv[]) {

	//Check if any arguments was given at startup
	if (argc > 1) {
		if (argv[1] == "-vs") {
			samplePath = "../../../";
		}
	}

	// Open a new window framework and set the title
	PandaFramework framework;
	framework.open_framework(argc, argv);
	framework.set_window_title("My Panda3D Window");

	// Open the window
	WindowFramework* window = framework.open_window();
	NodePath camera = window->get_camera_group(); // Get the camera and store it

	//Keyboard definitions
	window->enable_keyboard();

	snake mySnake(window, framework, samplePath);

	framework.main_loop();
	framework.close_framework();
	return 0;
}