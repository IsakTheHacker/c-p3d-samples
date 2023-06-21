#pragma once

//Panda3D includes
#include "pandaFramework.h"
#include "collisionHandlerQueue.h"

namespace Globals {
	extern PandaFramework framework;
	extern WindowFramework* window;
	extern LVector3 ballV;
	extern LVector3 accelV;
	extern PT(CollisionHandlerQueue) collision_handler;
	extern CollisionTraverser collision_traverser;
	extern PT(GenericAsyncTask) roll_task;
	extern NodePath maze;
	extern NodePath ball;
	extern NodePath ball_root;
	extern std::vector<NodePath> lose_triggers;
	extern NodePath title, instructions;

	//Some constants for the program
	extern const int ACCELERATION;
	extern const int MAX_SPEED;
	extern const int MAX_SPEED_SQ;
}
