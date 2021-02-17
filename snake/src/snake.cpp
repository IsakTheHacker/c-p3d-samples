#include "header/snake.h"

snake::snake(WindowFramework* window, PandaFramework& framework, std::string samplePath) {
	model = window->load_model(framework.get_models(), samplePath + "models/ball");
	model.reparent_to(window->get_render());
}

void snake::update() {
	if (direction == 1) {
	}
}