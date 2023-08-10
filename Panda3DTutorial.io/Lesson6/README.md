title: Lesson 6 (C++ Addendum)
------------------------------

The following expects you to first read:

[Lesson 6](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson06.html)

Collision traversers do not automatically get updated by the framework
when they are created.  Instead, they need to be updated in some
AsyncTask by calling its `traverse` method on some tree root (usually
`window->get_render()`).  Since we've already created one, the lazy
approach is used: it's updated at the start of that task.  It might be
better to just create a new task for this, since that way the priority
can be adjusted.

```c++
// In your include statements:
#include <panda3d/collisionTraverser.h>
```

```c++
// global:
WindowFramework *window; // remove "auto " from assignment in main()
CollisionTraverser c_trav;
```

```c++
// update() task:
c_trav.traverse(window->get_render());
```

Making a "pusher" is quite simple:

```c++
// In your include statements:
// note: since all the handler includes also include collisionTraverser.h,
// you can now remove that if you like.
#include <panda3d/CollisionHandlerPusher.h>
```

```c++
// in main()
auto pusher = new CollisionHandlerPusher;
```

To create the player collision solid (sphere):

```c++
// In your include statements:
#include <panda3d/collisionSphere.h>
```

```c++
// in main():
auto collider_node = new CollisionNode("player");
// Add a collision-sphere centred on (0, 0, 0), and with a radius of 0.3
collider_node->add_solid(new CollisionSphere(0, 0, 0, 0.3));
auto collider = temp_actor.attach_new_node(collider_node);
```

If you want to see your collision-object, just call "show" on the
collision-object's NodePath.

```c++
collider.show();
```

Finally, we tell both the traverser and the pusher that this is an
object that should collide with things, an "active" object:

```c++
// The pusher wants a collider, and a NodePath that
// should be moved by that collider's collisions.
// In this case, we want our player-Actor to be moved.
pusher->add_collider(collider, temp_actor);
// The traverser wants a collider, and a handler
// that responds to that collider's collisions
c_trav.add_collider(collider, pusher);
```

The "pusher" handler allows its responses to be restricted to the
horizontal, like so:

```c++
pusher->set_horizontal(true);
```

The process for adding wall collision solids is pretty much the same
as with creating the player's collision-object, but we're going to use
long capsule-shaped tubes instead of spheres. We also place the walls
in appropriate positions once we've created them.

```c++
// In your include statements:
#include <panda3d/collisionTube.h>
```

```c++
// In main():

// Tubes are defined by their start-points, end-points, and radius.
// In this first case, the tube goes from (-8, 0, 0) to (8, 0, 0),
// and has a radius of 0.2.
auto wall_solid = new CollisionTube(-8.0, 0, 0, 8.0, 0, 0, 0.2);
auto wall_node = new CollisionNode("wall");
wall_node->add_solid(wall_solid);
auto wall = render.attach_new_node(wall_node);
wall.set_y(8.0);

wall_solid = new CollisionTube(-8.0, 0, 0, 8.0, 0, 0, 0.2);
wall_node = new CollisionNode("wall");
wall_node->add_solid(wall_solid);
wall = render.attach_new_node(wall_node);
wall.set_y(-8.0);

wall_solid = new CollisionTube(0, -8.0, 0, 0, 8.0, 0, 0.2);
wall_node = new CollisionNode("wall");
wall_node->add_solid(wall_solid);
wall = render.attach_new_node(wall_node);
wall.set_x(8.0);

wall_solid = new CollisionTube(0, -8.0, 0, 0, 8.0, 0, 0.2);
wall_node = new CollisionNode("wall");
wall_node->add_solid(wall_solid);
wall = render.attach_new_node(wall_node);
wall.set_x(-8.0);
```

If we try to run around now, we'll find ourselves stopped at the walls!

With the basics of collision in place, it's time to lay the foundation
for our core gameplay...

Proceed to [Lesson 7](../Lesson7) to continue.
