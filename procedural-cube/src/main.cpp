#include "pandaFramework.h"
#include "pandaSystem.h"
#include "geomVertexFormat.h"
#include "geomVertexData.h"
#include "geomVertexWriter.h"

//You can't normalize inline so this is a helper function
LVector3 normalized(double x, double y, double z) {
	LVector3 myVec(x, y, z);
	myVec.normalize();
	return myVec;
}

int makeSquare(double x1, double x2, double z1, double z2, double y1, double y2) {
	CPT(GeomVertexFormat) format = GeomVertexFormat::get_v3n3cpt2();
	GeomVertexData vdata("square", format, Geom::UH_dynamic);

	GeomVertexWriter vertex(&vdata, "vertex");
	GeomVertexWriter normal(&vdata, "normal");
	GeomVertexWriter color(&vdata, "color");
	GeomVertexWriter texcoord(&vdata, "texcoord");

	if (x1 != x2) {
		vertex.add_data3(x1, y1, z1);
		vertex.add_data3(x2, y1, z1);
		vertex.add_data3(x2, y2, z2);
		vertex.add_data3(x1, y2, z2);

		normal.add_data3(normalized(2 * x1 - 1, 2 * y1 - 1, 2 * z1 - 1));
		normal.add_data3(normalized(2 * x2 - 1, 2 * y1 - 1, 2 * z1 - 1));
		normal.add_data3(normalized(2 * x2 - 1, 2 * y2 - 1, 2 * z2 - 1));
		normal.add_data3(normalized(2 * x1 - 1, 2 * y2 - 1, 2 * z2 - 1));
	} else {

	}
}

int main(int argc, char* argv[]) {
	// Open a new window framework
	PandaFramework framework;
	framework.open_framework(argc, argv);

	// Set the window title and open the window
	framework.set_window_title("My Panda3D Window");
	WindowFramework* window = framework.open_window();

	// Here is room for your own code

	// Do the main loop, equal to run() in python
	framework.main_loop();
	framework.close_framework();
	return (0);
}