#pragma once

//Panda3D includes
#include "ambientLight.h"
#include "directionalLight.h"
#include "collisionRay.h"
#include "load_prc_file.h"

//My includes
#include "header/globals.h"
#include "header/functions.h"

void setupLighting();
void setupMaterial();
void initPanda(int argc, char* argv[]);
void setupModels(std::string samplePath);