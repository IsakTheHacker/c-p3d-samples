 Translation of Python asteroids sample from Panda3D.

[Original](https://github.com/panda3d/panda3d/tree/v1.10.13/samples/asteroids):

>Author: Shao Zhang, Phil Saltzman, and Greg Lindley
>Last Updated: 2015-03-13
>
>This tutorial demonstrates the use of tasks. A task is a function that
>gets called once every frame. They are good for things that need to be
>updated very often. In the case of asteroids, we use tasks to update
>the positions of all the objects, and to check if the bullets or the
>ship have hit the asteroids.
>
>Note: This definitely a complicated example. Tasks are the cores of
>most games so it seemed appropriate to show what a full game in Panda
>could look like.
