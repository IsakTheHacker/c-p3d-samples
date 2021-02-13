#include "header/globals.h"

namespace Globals {
	PandaFramework framework;
	WindowFramework* window;
	LVector3 ballV(0, 0, 0);
	LVector3 accelV(0, 0, 0);
	PT(CollisionHandlerQueue) collision_handler = new CollisionHandlerQueue();
	NodePath maze = NodePath("maze");
	NodePath ball = NodePath("ball");
	NodePath ball_root = NodePath("ball_root");

	//Some constants for the program
	int ACCELERATION = 70;							//Acceleration in ft/sec/sec
	int MAX_SPEED = 5;								//Max speed in ft/sec
	int MAX_SPEED_SQ = 25;							//MAX_SPEED squared
}