#include "pandaFramework.h"
#include "pandaSystem.h"

//Some constants for the program
int ACCELERATION = 70;							//Acceleration in ft/sec/sec
int MAX_SPEED = 5;								//Max speed in ft/sec
int MAX_SPEED_SQ = MAX_SPEED * MAX_SPEED;		//MAX_SPEED squared

//Exit function
void panda_exit(const Event* theEvent, void* data) {
	exit(0);
}

int main(int argc, char* argv[]) {

	//Open framework
	PandaFramework framework;
	framework.open_framework(argc, argv);

	//Set the window title and open new window
	framework.set_window_title("Ball in maze - C++ Panda3D Samples");
	WindowFramework* window = framework.open_window();

	//Keyboard definitions
	window->enable_keyboard();
	framework.define_key("escape", "Escape-key", panda_exit, 0);

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}