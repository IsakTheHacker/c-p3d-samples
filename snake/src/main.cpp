#include "header/snake.h"
#include "header/tasks.h"
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

	snake mySnake(window, framework, samplePath);

	std::pair<snake*, int> up_key(&mySnake, 0);
	std::pair<snake*, int> right_key(&mySnake, 1);
	std::pair<snake*, int> down_key(&mySnake, 2);
	std::pair<snake*, int> left_key(&mySnake, 3);

	//Keyboard definitions
	window->enable_keyboard();
	framework.define_key("arrow_up", "Arrow Up-key", keypress, (void*)&up_key);
	framework.define_key("arrow_right", "Arrow Right-key", keypress, (void*)&right_key);
	framework.define_key("arrow_down", "Arrow Down-key", keypress, (void*)&down_key);
	framework.define_key("arrow_left", "Arrow Left-key", keypress, (void*)&left_key);

	//Setup tasks
	PT(GenericAsyncTask) moveTask = new GenericAsyncTask("roll_task", move, (void*)&mySnake);
	AsyncTaskManager::get_global_ptr()->add(moveTask);

	framework.main_loop();
	framework.close_framework();
	return 0;
}