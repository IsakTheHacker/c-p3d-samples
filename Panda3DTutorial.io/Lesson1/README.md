Lesson 1 (C++ Addendum)
-----------------------

The following expects you to first read:

[Lesson 1](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson01.html)

Let me start out with a few fundamental differences between the Python
API and the C++ API.

The Direct framework is a Python wrapper around the core C++ Panda3D
API.  It is not possible to use any of it directly in C++, unless you
re-implement it yourself.  There are often C++ equivalents, which may
be easier or (more likely) harder to use than Direct.  In particular,
ShowBase, which most sample Python code starts from, is replaced by
PandaFramework, which works a little differently.  It does not
automatically open a window and set up a lot of convenient global
behavior; much of that must be done using API calls.

The next immediate difference is in the object method calls.
Panda3D's Python wrappers renamed (yes, renamed:  rememeber, the
Python interface is a wrapper around the C++ core Panda3D library) all
of the so-called "snake case" methods (i.e., lower-case with
underscores between words) to so-called "camelCase" (i.e., start with
a lower-case letter, and separate words by upper-casing the first
letter of a word).  As a compromise, it is (at least at the time of
writing) now possible to use the "snake case" equivalents now in
Python, as well, but nobody seems to do that.  So, for C++ code, you
will have to translate all of those "camelCase" names to "snake case".
But not the class names themselves!  These are "CamelCase" (the same
as "camelCase", but with a leading capital letter).  In fact, given
the need to translate all API methods and members to "snake case", I
have done the same thing to all of the tutorial's code.  I do not
prefer one style over the other, but I do want to keep a consistent
style.

Of course Panda3D then violates this consistent style by naming their
include files after the class they define, but with an initial
lower-case letter.  This leads to another change from the Python
version:  the include files are not organized in a hierarchy:  they
are all in a single global path.  Most C++ code examples you will see
load an include file as e.g. `#include "pandaFramework.h"`, and assume
that you have included the path all the way to that file in your
compiler's include path.  Since the SDK, as installed on Linux, places
these include files in a subdirectory of their own, at least, I have
written all of my code to use `panda3d/` as a prefix.  Furthermore, it
is conventional to refer to libraries' include files with angle
brackets, so instead of the above, you will see my code using
`#include <panda3d/pandaFramework.h>`.  It's much easier to tell that
it's a Panda3D-specific library include file.  If you don't like this,
feel free to revert to the official method.

After all that long-winded explanation, the framework is easy to
start up, but of course, as mentioned above, the framework won't
always hold your hand, so you'll have to open a window yourself, using
`open_window`.  In fact, the constructor does so little that you have
to call `open_framework` after creating the framework object.

```cpp
#include <panda3d/pandaFramework.h>

int main(void)
{
    PandaFramework framework;
    framework.open_framework();
    auto window = framework.open_window();

    framework.main_loop();

    framework.close_framework();
    return 0;
}
```

If you run the code above, a window should open titled "Panda", and
showing an empty grey view.

Now, of course, I did not create a subclass of PandaFramework, like
the Python code did.  You can feel free to do that, but there is far
less advantage to doing that in C++ than in Python.  I prefer to think
of the mainline as an object of its own, so I don't use an object to
encapsulate the entire program.  Again, this is more of a sytlistic
difference, rather than a hard difference.

There might be an advantage to subclassing the window's class, since
that's where most of the equivalent globals from ShowBase reside, but
it's meant to only be created with `open_window`, so you're stuck with
the base class.

Now, on to resizing the window.  As mentioned in the Python tutorial,
there are several ways to do this.  Here is the equivalent to what the
Python tutorial used.  The first thing you may notice is that instead
of `win`, you have to use `window->get_graphics_window()`.  Get used
to it.  The next thing you will need to deal with is that Python code
always creates objects in the same way.  In the C++ interface, some
objects can be created directly, and some must be allocated.  There is
a way to avoid allocation, but I will not be covering that yet.  For
the `WindowProperties` object, allocating on the stack is fine.

```c++
#include <panda3d/pandaFramework.h>

PandaFramework framework;

int main(void)
{
    framework.open_framework();
    auto window = framework.open_window();
    
    WindowProperties properties;
    properties.set_size(1000, 750);
    auto win = window->get_graphics_window();
    win->request_properties(properties);

    framework.main_loop();
    framework.close_framework();
    return 0;
}
```

However, one thing I don't like about the above code is that it takes
effect *after* the window has been opened.  This is especially
noticable when you load assets, as the change will not take effect
until the first frame is rendered.  Instead, a similar method can be
used to set the window properties upon opening.  The trick is to
modify the framework's default properties, and pass those into the
open call:

```c++
#include <panda3d/pandaFramework.h>

PandaFramework framework;

int main(void)
{
    framework.open_framework();
    WindowProperties properties;
    framework.get_default_window_props(properties);
    properties.set_size(1000, 750);
    auto window = framework.open_window(properties, 0);

    framework.main_loop();
    framework.close_framework();
    return 0;
}
```

Note that because the PandaFramework does not create mouse controls by
default, there is no need to disable them.  If you do want to *enable*
them, though, you can use the following, although this tutorial uses
its own controls, so don't do that:

```c++
window->setup_trackball();
```

Proceed to [Lesson 2](../Lesson2) to continue.
