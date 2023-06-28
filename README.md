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
particular Python or Python-like languages or Python-like practaices
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
most defintely not tutorial in nature, but, again, it's better than
nothing.  Don't even bother looking at the broken latex output (20k+
pages of bloated garbage, and then dies).  Perhaps after I get things
working on the code side, assuming I don't abandon Panda3D entirely in
frustration, I'll revisit this and clean up the documentation.  This
really needs to be coordinated with the official maintainers, though.

TODO:

 - ✓ asteroids
 - ✓ ball-in-maze
 - X boxing-robots - Compiles, but punch animations silently missing
 - ✓ bump-mapping
 - ✓ carousel
 - _ cartoon-shader - requires apparently Python-only functionality
 - ✓ chessboard
 - ✓ culling
 - ✓ disco-lights
 - _  distortion - requires apparently Python-only functionality
 - _  fireflies
 - _  fractal-plants
 - _  gamepad
 - _  glow-filter
 - _  infinite-tunnel
 - ✓  looking-and-gripping
 - ✓  media-player
 - _  motion-trails
 - _  mouse-modes
 - ✓  music-box - gives lock assertion error on exit, but otherwise OK
 - _  particles
 - _  procedural-cube
 - _  render-to-texture
 - _  roaming-ralph
 - _  rocket-console
 - ✓  shader-terrain
 - _  shadows
 - ✓  solar-system
