
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

	//Start game
	new_game();
	
	window->get_render().ls();

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}