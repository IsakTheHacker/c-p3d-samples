#include "header/tasks.h"

AsyncTask::DoneStatus move(GenericAsyncTask* task, void* data) {
	
	snake mySnake = *(snake*)data;
	mySnake.update();

	return AsyncTask::DS_cont;
}

void keypress(const Event* theEvent, void* data) {

	std::pair<snake*, int>* input = (std::pair<snake*, int>*)data;

	input->first->direction = input->second;
}