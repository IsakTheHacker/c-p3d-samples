---
layout: page
title: Lesson 3 (C++ Addendum)
---
The following expects you to first read:

[Lesson 3](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson03.html)

You can create an ambient light like so:
```c++
// In your include group
#include <panda3d/ambientLight.h>
```

```c++
// In the body of your code--again, "main" will do in this case:
auto ambient_light = new AmbientLight("ambient light");
ambient_light->set_color(LColor(0.2, 0.2, 0.2, 1));
```

Note that LColor is the preferred type to use for colors.  If you want
something shorter, and a color is expected, just use an initializer
list, like `{0.2, 0.2, 0.2, 1}`.

To attach your ambient light to a node path:

```c++
auto ambient_light_node_path = render.attach_new_node(ambient_light);
```

To attach it to the main scene (aliased to `render` earlier):

```c++
render.set_light(ambient_light_node_path);
```

All in all, the code looks like this:
```c++
auto ambient_light = new AmbientLight("ambient light");
ambient_light->set_color(LColor(0.2, 0.2, 0.2, 1));
auto ambient_light_node_path = render.attach_new_node(ambient_light);
render.set_light(ambient_light_node_path);
```

To add directional light:
```c++
// In your include statements:
#include <panda3d/directionalLight.h>
```

```c++
// In the body of your code
auto main_light = new DirectionalLight("main light");
auto main_light_node_path = render.attach_new_node(main_light);
// Turn it around by 45 degrees, and tilt it down by 45 degrees
main_light_node_path.set_hpr(45, -45, 0);
render.set_light(main_light_node_path);
```

Activating automatic shaders is just as easy as with Python:

```c++
render.set_shader_auto();
```

Of course, this game doesn't do much yet. Let's change that next...

Proceed to Lesson4 to continue.
