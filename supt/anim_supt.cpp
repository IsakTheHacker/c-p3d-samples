#include "anim_supt.h"
#include <cIntervalManager.h>
#include <genericAsyncTask.h>
#include <asyncTaskManager.h>

// FIXME: sleep until there's something to do
void init_interval(void)
{
    AsyncTaskManager::get_global_ptr()->
	add(new GenericAsyncTask("cInterval", [](GenericAsyncTask *, void *){
	    CIntervalManager::get_global_ptr()->step();
	    return AsyncTask::DS_cont;
    }, 0));
}
