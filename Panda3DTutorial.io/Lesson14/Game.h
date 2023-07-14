#include <panda3d/windowFramework.h>
#include <panda3d/collisionHandlerPusher.h>

#include "GameObject.h"

// externs declared in main
extern WindowFramework *window;
extern CollisionTraverser c_trav;
extern PT(CollisionHandlerPusher) pusher;
extern PT(AudioManager) music_manager, sfx_manager;

enum { k_up, k_down, k_left, k_right, k_shoot };
