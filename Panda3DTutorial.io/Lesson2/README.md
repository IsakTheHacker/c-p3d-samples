---
layout: page
title: Lesson 2 (C++ Addendum)
---
The following expects you to first read:

[Lesson 2](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson02.html)

While the C++ API also provides a number of globally accessible
varialbes, their names aren't nearly as short.  As already shown in
the first exmaple, `win` is stored as `window->get_graphics_window()`.
Many classes have a single, global object that is intended to be used
most often.  This is often obtained by the static `get_global_ptr()`
method.  Sometimes, you can get there via shorter paths, but in the
case of the global loader, it's `Loader::get_global_ptr().`.  Since
that's a mouthful, it's sometimes best to make an alias.

```c++
auto loader = Loader::get_global_ptr();
```

Here we get into another fundamental difference in the Python code, as
well.  Some things are structures, and some things are pointers to
structures.  Python uses `.` as an accessor for everything, but in
C++, you will need `->` for some (actually most) accesses.  The loader
is one of them.

To then load the model, we could use the loader:

```c++
loader->load_model("Models/Misc/environment");
```

Rather than using the global loader to load models, I generallly use
`window->load_model`.  This method adds some virtual filesystem and
search features, but they aren't that useful for this project so far. 
The only thing useful for this project is that the `reparent_to` is no
longer necessary, as the parent is passed as the first parameter.  The
parent is the oft-used (for obvious reasons) `render` global variable,
which in C++ is obtained via `window->get_render()`.  This is a
reference, so it can't be easily used as a global.  Instead, I
re-alias every time I use it more than once in a particular scope.

```c++
auto render = window->get_render(); // NodePath &
window->load_model(render, "Models/Misc/environment");
```

One additional note, though: the lack of an absolute path makes
Panda3D look for the assets in the default search path.   The default
value generally includes some system directories and the exeutable
location, which changes for every lesson executable if executed
without installation.  It is best to store all the assets in one place
so that Panda3D's cache works correctly.  This place can simply be
added to the model path with a prc file containing:

>     model-path /my/path/to/assets

You can place this in a system prc file, but I recommend creating a
PRC file (e.g. `assets.prc`) in `/my/path/to/assets` itself, containing
this line, instead, which adds whatever directory the PRC file came
from (and can therefore be moved without having to edit the file):

>     model-path ${THIS_PRC_DIR}

Then, make sure Panda3D will search for PRC files there, by setting
the `PANDA_PRC_PATH` environment variable to both the system PRC file
directory and `/my/path/to/assets`, separated by a semicolon on Windows
and a colon elsewhere.  For example, on my system:

```sh
export PANDA_PRC_PATH=/etc:/my/path/to/assets
```

Actors are a Direct concept, not available in C++.  Animated
characters are just "models", like the environment.  In fact, even
animations are "models".  A "model" is simply another name for a
pre-populated scene graph.  The node types are what affect the actual
functionality.  The node type for animated models is Character (in
both APIs), so I prefer calling them characters.

Loading the main character model is just like the environment:

```c++
auto temp_actor = window->load_model(render, "Models/PandaChan/act_p3d_chan");
```

The animation must not only be loaded, it must be *bound*.  Binding
results in an animation controller that applies the animation to the
animated model.  If your animations and models are structured the way
Panda3D likes it, you can do this very easily.  Just load the
animation(s) and auto-bind.  Since animations are not affected by the
render process, they can be placed anywhere.  A child of the animated
model seems appropriate.  Even more appropriate might be someplace
entirely outside of the main rendering scene, but that's an issue for
another time.

```c++
#include <panda3d/auto_bind.h>
//...
window->load_model(temp_actor, "Models/PandaChan/a_p3d_chan_run");
AnimControlCollection anims;
auto_bind(temp_actor.node(), anims);
auto walk = anims.get_anim(0);
```

However, this animation does not conform to Panda3D's restrictive
expectations:  nothing is bound.  It fails silently, so you have to
enable some debugging messages to see why:

```c++
Notify::ptr()->get_category(":chan")->set_severity(NS_spam);
```

The gist of it is:

```
:chan: Part skelington has 1 children, while matching anim node has 2:
:chan:   anim has Bip001 Footsteps, not in part.
:chan: Bind failed.
```

But, there is hope:  you can pass in a hierarchy matching flag to
disable that check, and then it works:

```c++
auto_bind(temp_actor.node(), anims, PartGroup::HMF_ok_anim_extra);
```

Note that I assigned the result to a named variable.  Python
encourages the use of dictionaries, which are much less efficient than
named variables or even indexed arrays (which can use enumerated
constants to retain the "named" feel).  However, the
AnimControlCollection class supports access by name.  Of course due to
the silly restriction that the animation name must match the character
name, the auto-binding process doesn't really support this.  However,
it can be coerced into working by adding yet another obnoxiously long
hierarchy validation override flag.  This also requires overriding the
bundle name with the desired name before auto-binding, though.  If you
think it's worth it, assign the load result to a variable, say `n`,
and then replace the `auto_bind` call with:

```c++
DCAST(AnimBundleNode, n.get_child(0).node())->get_bundle()->set_name("walk");
auto_bind(temp_actor.node(), anims,
          PartGroup::HMF_ok_anim_extra |
          PartGroup::HMF_ok_wrong_root_name);
```

The first line assumes that the first child of the model is the
AnimBundleNode.  It is for all of the animations we'll be using.  Its
asociated bundle is retrieved with `get_bundle`.  That's whose name we
want to change.  I will not explain the DCAST here; see Lesson 8 for
what that is and why I used it.  The fourth line is the new flag to
avoid failure to bind if the bundle name doesn't match the model name.

To access this animation using the variable, just use `walk`.  To
access this animation using the named animation, which most closely
resembles the Python Actor usage, use e.g. `anims.play("walk")`.  You
can also combine the two by assiging `walk` to
`anims.find_anim("walk")` to avoid future named lookups.  Since
there's only one animation, all of this naming is overkill.

Now, to move the character to where we can see it:

```c++
temp_actor.set_pos(0, 7, 0);
```

You should now see the Actor framed in the doorway of the environment model!

By the way, sometimes you'll get a model that doesn't face the direction that you intend. You could just rotate the model's NodePath--but that may complicate any rotations that you want to do later. As it happens, models aren't usually loaded as single nodes, but rather tend to have at least one child-node containing the models themselves. (This applies to both actors and non-actors.) Thus you can access this child-node, and rotate it, like so:

```c++
temp_actor.get_child(0).set_h(180);
```

With the Actor visible, let's animate it.  Unlike the Python API,
there is a non-optional parameter to enable restarting from the first
frame.  Set it to `false` if you want to continue a paused animation.

```c++
walk->loop(true);
```

You should now see Panda-chan running in place.

Once again, in order to set camera properties, you need to obtain the
"global" path from the window: `window->get_camera_group()` rather
than using a convenient short name like `camera`.  The reason it's a
group is that you might have stereo or otherwise multiple views into
the scene; the group is a root node controlling all cameras.  If you
want to access individual cameras, via their Camera node, use
`window->get_camera(n)`, where n is 0 for the "default" camera,
instead of Python's much shorter `cam`.

Remove the call to "set_pos" that we added above (returning the
character to the centre of the scene), and instead add this:

```c++
// Move the camera to a position high above the screen
// --that is, offset it along the z-axis.
auto camera = window->get_camera_group(); // a shorter alias
camera.set_pos(0, 0, 32);
// Tilt the camera down by setting its pitch.
camera.set_p(-90);
```

But it still all looks so flat. Let's fix that...

Proceed to Lesson3 to continue.
