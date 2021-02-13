
#include "header/init.h"

std::string samplePath = "./";

int main(int argc, char* argv[]) {
	using namespace Globals;

	//Check if any arguments was given at startup
	if (argc > 1) {
		if (argv[1] == "-vs") {
			samplePath = "../../../";
		}
	}

	initPanda(argc, argv);
	setupModels(samplePath);
	setupLighting();			//Lighting
	setupMaterial();			//Material for the ball

	//More initialization
	ball_root.set_pos(maze.find("**/start").get_pos());
	PT(GenericAsyncTask) roll_task = new GenericAsyncTask("roll_task", roll_func, (void*)window->get_mouse().node());
	AsyncTaskManager::get_global_ptr()->add(roll_task);
	
	window->get_render().ls();

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}