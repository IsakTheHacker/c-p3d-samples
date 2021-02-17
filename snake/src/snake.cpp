#include "header/snake.h"

snake::snake(WindowFramework* window, PandaFramework& framework, std::string samplePath) {
	model = window->load_model(framework.get_models(), samplePath + "models/ball");
	model.reparent_to(window->get_aspect_2d());
}

void snake::update() {
	if (direction == 0) {
		model.set_z(model, 0.01);
	} else if (direction == 1) {
		model.set_x(model, 0.01);
	} else if (direction == 2) {
		model.set_z(model, -0.01);
	} else if (direction == 3) {
		model.set_x(model, -0.01);
	}
}