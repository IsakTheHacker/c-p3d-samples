#include <panda3d/asyncTaskManager.h>
#include <panda3d/genericAsyncTask.h>
#include "sample_supt.h"

void start_updater(const std::string &name, AsyncTaskFunc func, int sort)
{
    auto mgr = AsyncTaskManager::get_global_ptr();
    auto task = mgr->find_task(name);
    if(task)
	return; // already running
    task = new FuncAsyncTask(name, func);
    task->set_sort(sort);
    mgr->add(task);
}

bool kill_task(const std::string &name)
{
    auto mgr = AsyncTaskManager::get_global_ptr();
    auto tasks = mgr->find_tasks(name);
    if(tasks.size())
	mgr->remove(tasks);
    return !!tasks.size();
}
