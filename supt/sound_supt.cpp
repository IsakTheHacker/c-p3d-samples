#include <panda3d/audioManager.h>
#include "sample_supt.h"

void update_sounds(std::vector<AudioManager *> mgrs)
{
    start_updater("sounds updater", [=] {
	for(auto m: mgrs)
	    m->update();
	return AsyncTask::DS_cont;
    }, 60); // sort order from ShowBase
}

void kill_sounds()
{
    kill_task("sounds updater");
}
