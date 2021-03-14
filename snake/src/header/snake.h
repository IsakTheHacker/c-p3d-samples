#pragma once

//Panda3D includes
#include "pandaFramework.h"
#include "pandaSystem.h"

class snake {
public:
	int direction = 1;
	NodePath model = NodePath("model");

	snake(WindowFramework* window, PandaFramework& framework, std::string samplePath);

	void update();
};