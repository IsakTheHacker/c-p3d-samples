#pragma once

//Panda includes
#include "genericAsyncTask.h"
#include "snake.h"

AsyncTask::DoneStatus move(GenericAsyncTask* task, void* data);

void keypress(const Event* theEvent, void* data);