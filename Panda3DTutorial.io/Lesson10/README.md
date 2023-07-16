Lesson 10 (C++ Addendum)
------------------------

The following expects you to first read:

[Lesson 10](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson10.html)

We get the mouse-position like this:

```c++
// In the Player class definition in GameObject.h:
// This stores the previous position of the mouse,
// as a fall-back in case we don't get a good position
// on a given update.
LPoint2 last_mouse_pos;
```
```c++
// In Player::Player:
last_mouse_pos = LPoint2(0, 0);
```
```c++
// In the include group:
#include <panda3d/mouseWatcher.h>
```
```c++
// In the "update" method of the Player class:

// It's possible that we'll find that we
// don't have the mouse--such as if the pointer
// is outside of the game-window. In that case,
// just use the previous position.
auto mouse_watcher = DCAST(MouseWatcher, window->get_mouse().node());
LPoint2 mouse_pos;
if(mouse_watcher->has_mouse())
    mouse_pos = mouse_watcher->get_mouse();
else
    mouse_pos = last_mouse_pos;
```

When shutting down, the framework's task manager might run tasks when
the window has already been closed.  This makes some things not work,
such as `window->get_mouse()`.  To prevent a crash on shutdown, check
if the graphics window is still available before proceeding in the
main update task:

```c++
// Game.cpp, start of update task
if(!window->get_graphics_window())
    return AsyncTask::DS_done;
```

Since we're playing on a flat, horizontal level, we can make use of a
neat convenience function that Panda provides: its "Plane" class can
determine where a line, defined by two points, intersects it. That
will give us our 3D point.

```c++
// In GameObject.h's Player class definition:
LPlane ground_plane;
```
```c++
// In Player::Player:

// Construct a plane facing upwards, and centred at (0, 0, 0)
ground_plane = LPlane({0, 0, 1}, {0, 0, 0});
```
```c++
/ Then, in the "update" method of the "Player" class:
LPoint3 mouse_pos_3d(0, 0, 0), near_point, far_point;

// Get the 3D line corresponding with the 
// 2D mouse-position.
// The "extrude" method will store its result in the
// "near_point" and "far_point" objects.
window->get_camera(0)->get_lens()->extrude(mouse_pos, near_point, far_point);
// Get the 3D point at which the 3D line
// intersects our ground-plane.
// Similarly to the above, the "intersectsLine" method
// will store its result in the "mousePos3D" object.
auto render = window->get_render();
auto camera = window->get_camera_group();
ground_plane.intersects_line(mouse_pos_3d,
                             render.get_relative_point(camera, near_point),
                             render.get_relative_point(camera, far_point));
```

Notice in the above code another global Python convenience variable
that's much harder to get to.  To get the primary lens, you have to
retrieve the primary camera (`get_camera(0)`) and then get its lens
(`get_lens`).

An aside by TJM: unlike the original author, I prefer stronger typing
over weaker typing.  I am in favor of Panda3D's distinction between
"points" and "vectors".  Now, if C++ only had return type overloading
(yes, I know it's possible to awkwardly fake it using type conversion
operator wrappers), things would be almost perfect.  C++ still suffers
from verbosity issues since C++-11, unfortunately.  More reserved
words than Ada, a language originally derided for its excess of
reserved words.  Awkward constructions intended to discourage
practices, such as all the different ways to do type casting, and the
awkward (and officially discouraged) way to do namespace inclusion.
Not to mention C++ authors wholeheartedly adopting this verbosity
design, with excessively long names and every name requiring too many
obnoxiously long selector prefixes (at least in part due to the lack
of return type overloading, in the case of enumeration constants).
Not only is this verbosity hard on the fingers when typing (alleviated
by editors with auto-completion, I suppose), it's hard on the eyes
when reading (for which the only alleviation is syntax highlighting,
which doesn't really help all that much).  (*end rant*)

Finally, now that we have the point that we want, we can use it to
direct our ray and orient our character.

```c++
// In the Player class definition of GameObject.h:
LVector2 y_vector;
```
```c++
// In Player::Player:

// This vector is used to calculate the orientation for
// the character's model. Since the character faces along
// the y-direction, we use the y-axis.
y_vector = LVector2(0, 1);
```
```c++
// And in the "update" method of the "Player" class:

LVector3 firing_vector(mouse_pos_3d - actor.get_pos());
auto firing_vector_2d = firing_vector.get_xy();
firing_vector_2d.normalize();
firing_vector.normalize();

auto heading = y_vector.signed_angle_deg(firing_vector_2d);

actor.set_h(heading);

if(firing_vector.length() > 0.001) {
    ray->set_origin(actor.get_pos());
    ray->set_direction(firing_vector);
}

last_mouse_pos = mouse_pos;
```

Of course, it's not fair that the walking enemy can be hit by both us
_and_ the traps, but can't do anything itself. So let's give it the
ability to attack, too...

Proceed to [Lesson 11](../Lesson11) to continue.
