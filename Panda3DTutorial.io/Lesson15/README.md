Lesson 15 (C++ Addendum)
------------------------

As mentioned several times before, Direct GUI is not available in C++.
There is PGUI, which has very few widgets and lacks other
functionality used by this game's GUI.  The very first object created
has no direct equivalent in PGUI, and, in fact, one optional parameter
to that (`fade=0.4`) pulls in yet another complex class not available
in C++ (Transitions).

There are several ways to solve this.

One would be to re-implement all of the DirectGUI features being used
by this code.  That is well beyond the scope of this tutorial, but I
could just wave my hands and say, "Here you go, use it.  Maybe even
look at it.  Just don't expect me to document it in this README."  It
would still require much more effort than I intended to put into this.

Another would be to implement the GUI "well enough", in that it misses
some features (such as the fade, that I don't even notice), but looks
essentially like the original.  Of course this violates the feature
parity promise I made at the start, but I made that promise before i
actually looked at the coe.

Another option would be to ignore Panda3D's GUI entirely, and use
something else.  Panda3D does support several other GUIs, but only in
the Python interface (most notably librocket, which is also in C++,
but apparently Panda3D can only use the Python interface which uses
the obsolete Python version 2).  I have experimented with using [an
external Dear ImGui module for
Panda3D](https://github.com/bluekyu/panda3d_imgui) to replace the GUI
in the official musc-box sample, but I backed out later (removed after
[4f99f9a4d6449589f1cc812b7164d7cc645aaecc](https://github.com/darktjm/c-p3d-samples/tree/4f99f9a4d6449589f1cc812b7164d7cc645aaecc)).
Dear ImGui has many features, and is generally adequate, but it won't
look the same, obviously.  I honestly thing a third party GUI is the
best solution, since a fully functional GUI is not something I think
Panda3D's development team should be wasting its time on.

For now, I have done nothing, since all of these solutions would take
way too much time.  Besides, it's not as though anybody will ever read
this far.  Python is the way to go.  Oh, wait, they're planning on
getting rid of Direct.  What's supposed to replace it?  Magic?
Certainly not PandaFramework and PGUI.
