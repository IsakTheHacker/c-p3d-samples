---
layout: page
title: Lesson 5 (C++ Addendum)
---
The following expects you to first read:

[Lesson 5](https://arsthaumaturgis.github.io/Panda3DTutorial.io/tutorial/tut_lesson05.html)

The task system in the C++ API uses AsyncTasks, and its manager is the
AsyncTaskManager.  The default global, framework-managed one is
retrieved with `framework.get_task_mgr` (rather than the Python
taskMgr variable).  The expected, C++ way to create custom tasks is to
derive from AsyncTask, and override the `do_task` method.  However,
there is also a GenericAsyncTask, which executes a standard C function
pointer with `void *` user data.

There is no equivalent to Python's domethodLater.  Instead, you are
epxected to create a task to execute your method (via either of the
above two approaches) and then delay its start time with `set_delay`
before adding the task.

We're going to use the task-manager to run an update loop, calling a
function named "update".  Since we used a local variable for
`key_map`, that will be passed in as user data.

```c++
// In include group
#include <panda3d/genericAsyncTask.h>
```

```c++
// In main()
auto update_task = new GenericAsyncTask("update", update, key_map);
framework.get_task_mgr().add(update_task);
```

```c++
// Elsewhere:
AsyncTask::DoneStatus update(GenericAsyncTask *, void *)
{
    return AsyncTask::DS_cont;
}
```

Of course if update is defined after main, its function prototype has
to appear before main, at least.

The return type is an enumerated type, not a class member returning a
constant value, so even if we gave the task parameter a name (which I
didn't because giving it a name may return an unused parameter
warning), we couldn't use it to find the value of `cont`.  Instead,
use the type parent (AsyncTask), a type member selector (`::` rather
than `.`) and the enumeration name, which is prefixed by the the
initials of the enumerated type, followed by an underscore (i.e.,
DoneStatus becomes `DS_`).  You will see this change of convention
often when looking at Python code, although sometimes the Python
version of the constant has the prefix without the underscore (e.g.
DSCont).

Rather than pass a complex structure into the user data, I moved the
`temp_actor` out of main into the global variables.  In fact, if it
weren't for my reluctance to remove it, I would make `key_map` a
global as well.  Just obtain the value from the "global" object.

```c++
// before main()
NodePath temp_actor;  // remove the "auto " from the assignment in main()
AsyncTask::DoneStatus update(GenericAsyncTask *task, void *data);
```

```c++
// after main()
AsyncTask::DoneStatus update(GenericAsyncTask *, void *data)
    // Get the amount of time since the last update
    auto dt = ClockObject::get_global_clock()->get_dt();

    // If any movement keys are pressed, use the above time
    // to calculate how far to move the character, and apply that.
    auto key_map = reinterpret_cast<bool *>(data);
    if(key_map[k_up])
        temp_actor.set_pos(temp_actor.get_pos() + LVector3(0, 5.0*dt, 0));
    if(key_map[k_down])
        temp_actor.set_pos(temp_actor.get_pos() + LVector3(0, -5.0*dt, 0));
    if(key_map[k_left])
        temp_actor.set_pos(temp_actor.get_pos() + LVector3(-5.0*dt, 0, 0));
    if(key_map[k_right])
        temp_actor.set_pos(temp_actor.get_pos() + LVector3(5.0*dt, 0, 0));
    if(key_map[k_shoot])
        std::cout << "Zap!\n";

    return AsyncTask::DS_cont;
}
```

Removing the print statement from the key bindings simplifies the
macro definition a bit:

```c++
#define key_setter(k, v) EVH(kmap) { ((bool *)kmap)[k] = v; }, key_map
```

Of course, right now we can just run through the walls. That won't do...

Proceed to Lesson6 to continue.
