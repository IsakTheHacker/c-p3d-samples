#include <panda3d/cIntervalManager.h>
#include <panda3d/partBundle.h>
#include <panda3d/asyncTaskManager.h>
#include <panda3d/genericAsyncTask.h>
#include "sample_supt.h"

void update_intervals()
{
    start_updater("CInterval updater", [] {
	CIntervalManager::get_global_ptr()->step();
	return AsyncTask::DS_cont;
    }, 20); // sort order from ShowBase
}

void kill_intervals()
{
    kill_task("CInterval updater");
    auto mgr = CIntervalManager::get_global_ptr();
    for(unsigned i = mgr->get_max_index(); i--; ) {
	auto ci = mgr->get_c_interval(i);
	if(ci)
	    ci->finish();
    }
    while(mgr->get_num_intervals())
	mgr->step();
}

void FuncAsyncI::priv_instant()
{
    AsyncTaskManager::get_global_ptr()->
	add(new GenericAsyncTask("AsyncInterval", [](GenericAsyncTask *, void *v) {
	    reinterpret_cast<decltype(this)>(v)->_f();
	    return AsyncTask::DS_done;
	}, this));
}
