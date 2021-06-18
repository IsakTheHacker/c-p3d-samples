#include "pandaFramework.h"
#include "pandaSystem.h"
#include "geomVertexFormat.h"
#include "geomVertexData.h"
#include "geomVertexWriter.h"
#include "geomTriangles.h"
#include "geom.h"
#include "texturePool.h"
#include "spotlight.h"
#include "asyncTaskManager.h"


//Keyboard events
void toggleTex(const Event* theEvent, void* data) {
	std::pair<NodePath, PT(Texture)> objects = *(std::pair<NodePath, PT(Texture)>*)data;
	if (objects.first.has_texture()) {
		objects.first.set_texture_off(1);
	} else {
		objects.first.set_texture(objects.second);
	}
	std::cout << "toggle tex called!" << std::endl;
}
void toggleLights(const Event* theEvent, void* data) {
	std::tuple<NodePath, NodePath, bool*, NodePath> objects = *(std::tuple<NodePath, NodePath, bool*, NodePath>*)data;
	*std::get<2>(objects) = !*std::get<2>(objects);

	if (std::get<2>(objects)) {
		std::get<3>(objects).set_light(std::get<1>(objects));
		std::get<1>(objects).set_pos(std::get<0>(objects), 10, -400, 0);
		std::get<1>(objects).look_at(10, 0, 0);
	} else {
		std::get<3>(objects).set_light_off(std::get<1>(objects));
	}
	std::cout << "toggle lights called!" << std::endl;
}

// This task runs for two seconds, then prints done
AsyncTask::DoneStatus rotateTask(GenericAsyncTask* task, void* data) {
	NodePath cube = *(NodePath*)data;
	cube.set_hpr(cube.get_hpr() + 250 * AsyncTaskManager::get_global_ptr()->get_clock()->get_dt());
	/*cube.hprInterval(1.5, (360, 360, 360)).loop()*/
	return AsyncTask::DS_cont;
}

//You can't normalize inline so this is a helper function
LVector3 normalized(double x, double y, double z) {
	LVector3 myVec(x, y, z);
	myVec.normalize();
	return myVec;
}

PT(Geom) makeSquare(double x1, double x2, double z1, double z2, double y1, double y2) {
	CPT(GeomVertexFormat) format = GeomVertexFormat::get_v3n3cpt2();
	PT(GeomVertexData) vdata = new GeomVertexData("square", format, Geom::UH_dynamic);

	GeomVertexWriter vertex(vdata, "vertex");
	GeomVertexWriter normal(vdata, "normal");
	GeomVertexWriter color(vdata, "color");
	GeomVertexWriter texcoord(vdata, "texcoord");

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
		vertex.add_data3(x1, y1, z1);
		vertex.add_data3(x2, y2, z1);
		vertex.add_data3(x2, y2, z2);
		vertex.add_data3(x1, y1, z2);

		normal.add_data3(normalized(2 * x1 - 1, 2 * y1 - 1, 2 * z1 - 1));
		normal.add_data3(normalized(2 * x2 - 1, 2 * y2 - 1, 2 * z1 - 1));
		normal.add_data3(normalized(2 * x2 - 1, 2 * y2 - 1, 2 * z2 - 1));
		normal.add_data3(normalized(2 * x1 - 1, 2 * y1 - 1, 2 * z2 - 1));
	}

	//Adding different colors to the vertex for visibility
	color.add_data4f(1.0, 0.0, 0.0, 1.0);
	color.add_data4f(0.0, 1.0, 0.0, 1.0);
	color.add_data4f(0.0, 0.0, 1.0, 1.0);
	color.add_data4f(1.0, 0.0, 1.0, 1.0);

	texcoord.add_data2f(0.0, 1.0);
	texcoord.add_data2f(0.0, 0.0);
	texcoord.add_data2f(1.0, 0.0);
	texcoord.add_data2f(1.0, 1.0);

	//Quads aren't directly supported by the Geom interface
	//you might be interested in the CardMaker class if you are
	//interested in rectangle though
	PT(GeomTriangles) tris = new GeomTriangles(Geom::UH_dynamic);
	tris->add_vertices(0, 1, 3);
	tris->add_vertices(1, 2, 3);

	PT(Geom) square = new Geom(vdata);
	square->add_primitive(tris);
	return square;
}

int main(int argc, char* argv[]) {

	//Open a new window framework
	PandaFramework framework;
	framework.open_framework(argc, argv);

	//Set the window title and open the window
	framework.set_window_title("My Panda3D Window");
	WindowFramework* window = framework.open_window();
	NodePath camera = window->get_camera_group();
	camera.set_pos(0, -10, 0);

	//Note: it isn't particularly efficient to make every face as a separate Geom.
	//instead, it would be better to create one Geom holding all of the faces.
	PT(Geom) square0 = makeSquare(-1, -1, -1, 1, -1, 1);
	PT(Geom) square1 = makeSquare(-1, 1, -1, 1, 1, 1);
	PT(Geom) square2 = makeSquare(-1, 1, 1, 1, -1, 1);
	PT(Geom) square3 = makeSquare(-1, 1, -1, 1, -1, -1);
	PT(Geom) square4 = makeSquare(-1, -1, -1, -1, 1, 1);
	PT(Geom) square5 = makeSquare(1, -1, -1, 1, 1, 1);
	GeomNode snode("square");
	snode.add_geom(square0);
	snode.add_geom(square1);
	snode.add_geom(square2);
	snode.add_geom(square3);
	snode.add_geom(square4);
	snode.add_geom(square5);
	
	NodePath cube = window->get_render().attach_new_node(DCAST(PandaNode, &snode));

	//OpenGL by default only draws "front faces" (polygons whose vertices are
	//specified CCW).
	cube.set_two_sided(true);

	//Init

	bool LightsOn = false;
	bool LightsOn1 = false;
	PT(Spotlight) slight = new Spotlight("slight");
	slight->set_color((1, 1, 1, 1));
	PerspectiveLens lens;
	slight->set_lens(DCAST(Lens, &lens));
	NodePath slnp = window->get_render().attach_new_node(slight);
	NodePath slnp1 = window->get_render().attach_new_node(slight);

	PT(Texture) testTexture = TexturePool::load_texture("models/envir-reeds.png");
	std::pair<NodePath, PT(Texture)> pair(cube, testTexture);
	std::tuple<NodePath, NodePath, bool*, NodePath> tuple(cube, slnp, &LightsOn, window->get_render());
	std::tuple<NodePath, NodePath, bool*, NodePath> tuple1(cube, slnp1, &LightsOn1, window->get_render());
	framework.define_key("s", "1-Key", toggleTex, (void*)&pair);
	framework.define_key("2", "2-Key", toggleLights, (void*)&tuple);
	framework.define_key("3", "3-Key", toggleLights, (void*)&tuple1);

	PT(GenericAsyncTask) task = new GenericAsyncTask("MyTaskName", &rotateTask, (void*)&cube);
	AsyncTaskManager::get_global_ptr()->add(task);

	//Do the main loop
	framework.main_loop();
	framework.close_framework();
	return 0;
}