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

This was a fork of https://github.com/IsakTheHacker/c-p3d-samples.  In
spite of the name and description, that was only one working example,
with missing features, and another mostly non-functional example,
which I have removed.  I appreciate that as a starter, since one of
the missing features of Panda3D's C++ support is anything resembling
usable documentation.  In the mean time, I have added feature parity
(on-screen help text and drain animation, along with all original code
comments), removed Microsoft build support (I don't develop on or use
Windows, so I have no way of modifying or testing; you might get a
Windows build using cmake), restructured the code to match the Python
code (all in one source file, in pretty much the same code order), and
removed the assets (you will need the assets from the original Python
sample).

This is my first Panda3D experience.  It's also my first serious
attempt at making something resembling a 3D application.  I wanted a
functional 3D library that supported my needs without major effort
required to get it running (e.g. pretty much everything I looked at,
including Panda3D, requires changes/fixes for animated model loading;
at least Panda3D's assimp support seems to be progressing without my
intervention), or major misfeatures (such as not being able to avoid
the use of excessive configuration files or scripting languages, in
particular Python or Python-like languages or Python-like practices
(e.g. auto-downloading a complete distribution in order to run
anything), as I consider Python a plague upon humanity, and an even
biggest step backwards than Visual Basic).  I originally rejected
Panda3D due to my impression that it was impossible to write anything
useful except in Python.  However, I found that most every other
freely available library is unusable, as well.  Upon re-examining it
just before deleting it, I found the C++ link in the official (on-line
only, impossible to download) documentation.  This is what encouraged
me to look into it further. This collection of samples represents my
work in evaluating the library.  I found that many things were still
missing, and expect that future development will involve even more
Python-only facilities, but it is tolerable as is.

As an aside, in order to get an offline (or any) document, I have
tried to make Doxygen produce something.  Even with projects that put
a lot of effort into it, Doxygen always produces awful documentation
(poorly organized at almost every level, massive excessive useless
copying and whitespace, but, hey, pretty (useless) pictures!), but
it's better than nothing.  Panda3D in particular uses practices that
make it even harder (I discovered many new misfeatures in Doxygen as a
result), such as placing documentation in the implementation rather
than in the API definition.  I have collected my current work on the
matter into the doxy directory.  It's intended to be run after
building, in a top-level subdirectory (doxy in my case) of the panda3d
source tree.  There are still undocumented entities, and it still
includes internal APIs probably not intended for public use, and it is
most definitely not tutorial in nature, but, again, it's better than
nothing.  Don't even bother looking at the broken latex output (20k+
pages of bloated garbage, and then dies).  Perhaps after I get things
working on the code side, assuming I don't abandon Panda3D entirely in
frustration, I'll revisit this and clean up the documentation.  This
really needs to be coordinated with the official maintainers, though.

TODO:

 - ✓ asteroids
 - ✓ ball-in-maze
 - ✓ boxing-robots
 - ✓ bump-mapping
 - ✓ carousel
 - ~ cartoon-shader - requires Python-only functionality; see note #2, #3.
 - ✓ chessboard
 - ✓ culling
 - ✓ disco-lights
 - ~  distortion - requires Python-only functionality; see note #3.
 - ~  fireflies - requires Python-only functionality; see note #3, #4.
 - ✓  fractal-plants
 - ~  gamepad - see note #5; also neither sample w/ GUI yet
 - _  glow-filter
 - _  infinite-tunnel
 - ✓  looking-and-gripping
 - ✓  media-player
 - _  motion-trails
 - ✓  mouse-modes
 - ✓  music-box - crashes on exit; see note #1
 - _  particles
 - _  procedural-cube
 - _  render-to-texture
 - _  roaming-ralph
 - _  rocket-console
 - ✓  shader-terrain
 - _  shadows
 - ✓  solar-system

notes:

1) OpenAL sounds require the OpenAL sound manager to exist when they
   are deleted.  At program exit, the deletion order can't be relied
   on.  I tried simple solutions to this, which didn't work, so I have
   left music-box to crash (assertion failure due to manager's lock no
   longer existing) on exit.  Really, the manager should track which
   sounds still exist, and delete them itself in its destructor.  Or
   at least flag the sounds to become inert.
2) The filter classes, including the "common filters" are only
   available in Python.  I suppose I should fix that, since they seem
   generally useful.  Maybe in a future revision.  Thus, the
   cartoon-shader "simple" sample won't work.
3) The BufferViewer class is a feature of the Direct GUI, and
   therefore not available in C++.  I have gone ahead and ported
   samples which originally used this without that particular
   sub-feature.  Maybe one day I'll port BufferViewer, but not now.
   though.  Way too much exposure to Python for my taste.
4) fireflies attempts to load models asynchronously, and often fails:
   seg faulting in the memory allocator (usually), not showing the
   "Loading models..." message, etc.  Sometimes it works, though, and
   it's hard to reproduce in the debugger or in valgrind.  I will
   revisit this some day, maybe.
5) I do not own a flight stick or steering wheel, and cannot test the
   ports of their respective samples.  It compiles, at least.
