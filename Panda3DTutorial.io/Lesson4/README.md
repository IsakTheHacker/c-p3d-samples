Lesson 4 (C++ Addendum)
-----------------------

The following expects you to first read:

[Lesson 4](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson04.html)

Note that events are not tied to objects in the C++ API.  Binding an
event to a callback is via the old C standard function pointer plus
`void *` user data.  The callback itself receives the event pointer
(`const Event *`) and the user data pointer as arguments.

Once again, rather than using a dictionary, I use an array here.  In
order to retain the index "names", I define an enumeration.

```c++
enum { k_up, k_down, k_left, k_right, k_shoot };
bool key_map[(int)k_shoot + 1] = {}; // {} initializes array to 0/false
```

If you use global variables and functions, there is no need to use the
user data.  In order to illustrate its usage, I have moved `key_map`
from a global into main.  The only way to pass locals is via the user
data pointer.  Often, if you want to access class methods or members,
you will be passing `this` or the primary object as the user data.
Since this is C++, not Python, you will have to cast the resulting
value in your handler back to the expected type.  Not having to
constantly do stuff like that is why I use globals, and if necessary,
simplifying macros in my own code.  I will not be presenting the
macros here, except one to simplify the addition of many handlers.
For very short code snippets, I much prefer using C++11 lambda
functions.

```c++
#define EVH(p) [](const Event *, void *p)
#define key_setter(k, v) EVH(kmap) { \
    ((bool *)kmap)[k] = v; \
    std::cout << #k " set to " << std::boolalpha << v << '\n'; \
  }, key_map
```

There is one global event handler, obtained most easily via
`framework.get_event_handler`.  This receives events from all sources,
and dispatches them to callbacks, assigned via so-called hooks.  These
hooks are registered using the handler's `add_hook` method.  As an
aside, there is a special version of event hook registration called
`framework.define_key`.  This takes an additional string argument
which is help text that is stored and displayed when its built-in help
text is desplayed.  Since we won't be using that built-in help text,
there is little advantage in using this.

For a single key-down event:

```c++
auto &evhand = framework.get_event_handler();
evhand.add_hook("w", key_setter(k_up, true));
```

For a single key-up event:

```c++
evhand.add_hook("w", key_setter(k_up, false));
```

Thus we end up with the following:

```c++
auto &evhand = framework.get_event_handler();
evhand.add_hook("w", key_setter(k_up, true));
evhand.add_hook("w-up", key_setter(k_up, false));
evhand.add_hook("s", key_setter(k_down, true));
evhand.add_hook("s-up", key_setter(k_down, false));
evhand.add_hook("a", key_setter(k_left, true));
evhand.add_hook("a-up", key_setter(k_left, false));
evhand.add_hook("d", key_setter(k_right, true));
evhand.add_hook("d-up", key_setter(k_right, false));
evhand.add_hook("mouse1", key_setter(k_shoot, true));
evhand.add_hook("mouse1-up", key_setter(k_shoot, false));
```

If you try this, you'll find that the keys do nothing.  This is
because, unlike ShowBase, PandaFramework doesn't enable everything by
default.  To get key events to even trigger, you have to do some
special magic which will not be covered here, except to give the
convenience function to do this magic:

```c++
window->enable_keyboard();
```

This still doesn't do much, of course. So, let's tie those controls to
a more interesting set of reponses...

Proceed to [Lesson 5](../Lesson5) to continue.
