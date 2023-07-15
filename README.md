# C++ Panda3D Samples

This is a collection of samples for Panda3D development in C++.
Panda3D officially only supports Python for game development.  Some
features of the library are only available in Python, such as many
animation facilities, the in-engine GUI, most of the documentation,
and all of the code samples.  There are also trivial differences
between the convenience frameworks (direct vs. PandaFramework).  I
have tried to match the code of the Python samples as much as
possible, within my tolerance of such things, but due to the missing
features, some things may be completely different.

To build any single sample, go into its directory, and build with cmake:

>     cmake -S . -B build
>     cmake --build build -j9

While each sample is essentially independent, they all depend on some
common support routines I created during the porting process.  These
are all in the `supt` subdirectory.  This includes the bulk of the
cmake code, so you can't even build without it.

You can also build all of the samples at once by executing the same
commands in the top-level directory.  While there is an install
target, it's generally not a good idea to install samples in the
system.  However, after building all samples at once, it might be
convenient to collect them all in one directory.  A way to do this is
to install.  For example, to install all of them in `/tmp/p3ds/bin`:

>     cmake --install build --prefix /tmp/p3ds

As is usual with CMake, solve build issues and/or set config options
using `ccmake` or `cmake-gui`.  In particular, note that if it can't
find the Panda3D libraries, it will give `NOTFOUND` for all of them,
but you only need to correct the first (PANDALIB, the path to the
`panda` library) and reconfigure to find the rest.

Every sample that has assets (i.e., most of them, including the
"procedurally generated" ones) takes a single command-line argument,
which is the directory in which to find the assets.  I did not
duplicate the sample assets here; you will have to obtain these from
the official SDK.  If the assets are found by CMake during the build
process, the sample will use the found directory by default, so you
should usually not need to provide this command-line parameter.

Official Panda3D 1.10.13 SDK samples
------------------------------------

Note that after I finished this work, I stumbled upon sombody else's
work from over 10 years ago, https://github.com/drivird/drunken-octo-robot.
That work is more complete in some ways: full reimplementing some of
the Direct classes in C++, and implementing the samples that I
skipped, along with BufferViewer.  Its seletion probably reflects what
was available in Panda3D 1.7.2, so it includes two texture swapping
tutorials, and doesn't include culling, fireball, fireflies,
fractal-plants, gamepad, mouse-modes, procedural-cube, shader-terrain,
or rocket-console.  Its only problem for me is obsolete C++ (requring
`using namespace std;` and ignoring deprecation warnings) and a few
Panda3D incompatibilities, probalby also due to obsolescence.  I also
don't care for Eclipse, so the build system is harder for me to use
than CMake (but easy enough to just replace with a simple shell
script). Given that project, though, I don't think the Panda3D team
has any excuse whatsoever for not shipping C++ versions of the samples
in the first place, saving me all this time, effort, and unwanted
Python exposure.

### Completely finished:

 - asteroids
 - ball-in-maze
 - boxing-robots
 - bump-mapping
 - carousel
 - chessboard
 - culling
 - disco-lights
 - fractal-plants
 - infinite-tunnel
 - looking-and-gripping
 - media-player
 - mouse-modes
 - music-box - see note #1.
 - procedural-cube
 - roaming-ralph
 - shader-terrain
 - solar-system

### Fully functional except no BufferViewer debug window (see note #3):

 - distortion
 - fireflies - sometims crashes; see note #4.
 - render-to-texture
 - shadows

### Partially finished; probably won't complete:

 - cartoon-shader - requires Python-only functionality; see note #2, #3.
 - motion-trails - fireball needs too much work; see note #8.
 - gamepad - see note #5; also neither sample w/ GUI yet (note #9)
 - glow-filter - requires Python-only functionality; see note #2, #3.

### Won't even start:

 - particles - both samples rely heavily on Python; see note #6.
 - rocket-console - requires librocket; see note #7.

### Notes:

1) OpenAL sounds require the OpenAL sound manager to exist when they
   are deleted.  At program exit, the deletion order can't be relied
   on.  I found a way to get it to shut down cleanly without an
   assertion failure on the managers' global locks, but I'm not very
   happy with the fix.  This probably needs fixing on the library
   level.  For now, music-box shuts down without a crash.  Note that
   the aforementioned previous effort crashes on exit as well.
2) The filter classes, including the "common filters" are only
   available in Python.  I suppose I should fix that, since they seem
   generally useful.  Maybe in a future revision.  Thus, the
   cartoon-shader and glow-filter "basic" samples won't work.  The
   "advanced" samples do everything manually, so they work.  Note that
   the aforementioned previous effort implemented the filter classes
   in C++.  I won't be copying them here/using them; if you want to
use them, just use his code.
3) The BufferViewer class is a feature of the Direct GUI, and
   therefore not available in C++.  I have gone ahead and ported
   samples which originally used this without that particular
   sub-feature.  Maybe one day I'll port BufferViewer, but not now.
   though.  Way too much exposure to Python for my taste.  Note that
   the afore-mentioned previous effort implemented this.  I will not copy
   that here; just use his code if you want to use it.
4) fireflies attempts to load models asynchronously, and often fails:
   seg faulting in the memory allocator (usually), not showing the
   "Loading models..." message, etc.  Sometimes it works, though, and
   it's hard to reproduce in the debugger or in valgrind.  I will
   revisit this some day, maybe.
5) I do not own a flight stick or steering wheel, and cannot test the
   ports of their respective samples.  It compiles, at least.
6) The particle samples will not be ported any time soon.  The
   particle_panel sample simply invokes the Python-only ParticlePanel
   class, which I don't feel like porting or examining to find out if
   there is a C++ equivalent.  The steam_example sample uses ptf files,
   which are Python scripts executed blindly by the loader.  I could
   just parse the parts that are generated, and reject any additional
   Python, or I could hand-convert these to C++ calls and compile them.
   I don't know of (and don't feel like looking for) a run-time
   loadable particle system format (maybe egg can be coerced?).  It
   would be nice if I could just write some Python code to do the
   conversion process, but I hate Python.  Note that the
   aforementioned previous effort did implement at least the steam
   example, simply hand-converting the ptf files to C++.  Again, I
   won't copy that code here; if you want to see it, use his code.
7) Panda3D's rocket support requires the Python API, which is currently
   officially Python2.  Thus, I can't compile Panda3D with it, and
   besides, given this requirement, I highly doubt I could do much
   with it outside of Python, anyway, or at the very least, I'd have
   a very hard time porting the Python code (see #6: the C++
   part is there, but it's not easy to use or well documented).
8) The fireball sample uses infrastucture which is technically available
   in C++, but requires some support and mangling, and basically
   duplication of the Python MotionTrail class.  Like #6 and #7, and
   Intervals, it's not well documented and hard to use outside of the
   Python wrappers.  The fact that it pulls in libp3direct should be a
   clear indicator it wasn't meant for raw usage in C++ code.
9) The PGUI elements are hard to use and poorly documented, much like
   #6-#8 above (seeing a trend?), and have some major features that I
   would need to re-implement in C++.  For these reasons, I may never
   port mappingGUI or device_tester.
0) ShowBase has a number of features which would be useful in the C++
   framework, as well.  Clearly the Python API gets more love,
   starting at the very beginning (ShowBase vs. PandaFramework), and
   extending through nearly everything.  I'm surprised when a port
   actually has close to the same number of lines as the Python code, and
   that's not entirely due to differences between the programming
   lanugages.  C++ just doesn't get enough love.  In some places, it
   is clearly only used to accelerate the Python API (e.g.
   MotionTrail). There are also some features of C++11 that dearly need
   to be implmented in the Panda3D classes, such as initializers and
   std::function support and proper container iterators that can be
   used in the for(x: container) construct.
0) The official C++ tutorial calls people lazy for using intervals,
   and then runs its character animations using `WindowFramework`'s
   `loop_animations` method.  I do not use that in any of my samples,
   because in the real world, the methods used by `loop_animations`
   don't work.  You'll want control over individual animations.
   You'll want to load multi-animation files with named animations
   that don't match your model's name, and be able to find, bind and
   run only the ones you want.  You'll want to be able to control the
   speed of individual animations.  In fact, even some of the samples
   I ported would not benefit from this sipmlistic method.  I suggest
   avoiding it in your own code, and I suggest to the Panda3D team to
   rethink or remove this method.  Was it created just to shorten the
   tutorial?  Is it really beneficial to users to call binding "magic"
   rather than providing the 3 or 4 lines of code needed to bind
   manually?  I guess with those obnoxiously long flag names needed to
   override the default matching behavior, maybe so.

Other samples
-------------

### Panda3DTutorial.io

This is a port of the tutorial referenced at the end of the official
C++ sample to C++.  The code has been ported up to Lesson 14 (15 uses
DirectGUI; see note #9 above).  The documentaion has not been ported;
instead, I have created addenda for C++, in the form of README.md
files, [displayable by Github](Panda3DTutorial.io).  The progress of
that is lagging, and only goes up to 11.

### square

This is a port of https://oldmanprogrammer.net/demos/square/.  It
generates a terrain with "square" tiles.  This was my first test
program for most other engines; for Panda3D, I waited until I finished
all the samples (in particular, fractal-tree and procedural-cube).
Mostly a quick hack.

Note that this is more independent than the above samples:  it does not
depend on the `supt` subdirectory at all (the cmake portions were
copied into the `CMakeLists.txt` itself).  The `square` directory can
be copied anywhere outside and still be built.  It can still be built
at the same time as the rest using a top-level build, though.

### skelanim

https://discourse.panda3d.org/t/hardware-skinning/14978

Other Notes
-----------

This was a fork of https://github.com/IsakTheHacker/c-p3d-samples.  In
spite of the name and description, that was only one working example
(ball-in-maze), with missing features, and another mostly
non-functional example ("snake"), which I have removed.  I appreciate
that as a starter, since one of the missing features of Panda3D's C++
support is anything resembling usable documentation (in the mean time,
I have found that the official documentation is not as bad as it
originally appeared to be, including an actual tutorial example, but
the fact that my initial impression was that it was a slightly
modified version of the Python documentation didn't just come out of
nowhere).  In the mean time, I have added feature parity with the
official Python version of ball-in-maze:  on-screen help text, drain
animation, and all original code comments.  I have also removed
Microsoft build support (I don't develop on or use Windows, so I have
no way of modifying or testing); CMake supports MS builds, so you might
get a Windows build, but I haven't tested it and won't:  if/when I do
build for Windows, I'll be using cross-compilation tools, anyway.  I
also restructured the code to match the Python code (all in one source
file, in pretty much the same code order) and removed the assets.  In
other words, it is almost a complete rewrite of what little there was.
Plus, of course, more than 20 additional samples mostly written from
scratch, including important concepts not present in the original,
like the animations and on-screen text I added to ball-in-maze.

This is my first Panda3D experience.  It's also my first serious
attempt at making something resembling a 3D application.  I wanted a
functional 3D library that supported my needs without major effort
required to get it running (e.g. pretty much everything I looked at,
including Panda3D, requires changes/fixes for animated model loading;
at least Panda3D's assimp support seemed to be progressing without my
intervention, but as evidenced by the responses to a bug report, I was
wrong:  their only asset importing solution is to use a third party
tool which has the misfeatures I mention next, and only works for
assets that blender can import properly), or major misfeatures (such
as not being able to avoid the use of excessive configuration files or
scripting languages, in particular Python or Python-like languages or
Python-like practices (e.g. auto-downloading a complete distribution
in order to run anything), as I consider Python a plague upon
humanity, and an even bigger step backwards than Visual Basic). I
originally rejected Panda3D due to my impression that it was
impossible to write anything useful except in Python.  However, I
found that most every other freely available library is unusable, as
well.  Upon re-examining it just before deleting it, I found the C++
link in the official documentation.  This is what encouraged me to
look into it further. This collection of samples represents my work in
evaluating the library.  I found that many things were still missing,
and expect that future development will involve even more Python-only
facilities, but it is tolerable as is.

As an aside, during development, I did not use the official API
reference.  Instead, I painstakingly, partially reconstructed it using
Doxygen, because I missed the fact that you can download the [official
documentation sources](https://github.com/panda3d/panda3d-docs) and
build them locally, albeit via a long, non-parallizable build
requiring `pip` and other things I dislike.  Not that I'm a big fan of
Doxygen, either, but it seemed like the best way to get a locally
installed API reference at the time.  I used to include my work to
date on this matter, but have removed it.  Feel free to scour the git
history for it if you are interested.
