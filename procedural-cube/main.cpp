/*
 *  Translation of Python procedural-cube sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/procedural-cube
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Kwasi Mensah (kmensah@andrew.cmu.edu)
 * Date: 8/02/2005
 *
 * This is meant to be a simple example of how to draw a cube
 * using Panda's new Geom Interface. Quads arent directly supported
 * since they get broken down to trianlges anyway.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/texturePool.h>
#include <panda3d/spotlight.h>
#include <panda3d/geomTriangles.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
PT(Texture) test_texture;
bool lights_on, lights_on1;
NodePath cube, slnp, slnp1;

// TJM: like other samples, this is the common code to add instruction text
void add_instruction(const std::string &msg, int off)
{
    auto a2d = window->get_aspect_2d();
    auto text_node = new TextNode("instruction");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    // style = 1 - default
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(-1.0/a2d.get_sx() + 0.06, 0, 1.0 - 0.08 - off * 0.06); // TopLeft == (-1,0,1)
    text_node->set_align(TextNode::A_left);
    text.set_scale(0.05);
}

void init_osd(void)
{
    framework.open_framework();
    framework.set_window_title("Procedural Cube - C++ Panda3D Samples");
    window = framework.open_window();

    window->get_camera_group().set_pos(0, -10, 0);

    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Making a Cube Procedurally");
    // style = 1 - default
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(1/a2d.get_sx()-0.1, 0, -1+0.1); // BottomRight = (1,0,-1)
    text.set_scale(0.07);
    text_node->set_align(TextNode::A_right);

    add_instruction("1: Set a Texture onto the Cube", 0);
    add_instruction("2: Toggle Light from the front On/Off", 1);
    add_instruction("3: Toggle Light from on top On/Off", 2);
}

// You can't normalize inline so this is a helper function
LVector3 normalized(PN_stdfloat x, PN_stdfloat y, PN_stdfloat z)
{
    LVector3 my_vec(x, y, z);
    my_vec.normalize();
    return my_vec;
}

// helper function to make a square given the Lower-Left-Hand and
// Upper-Right-Hand corners

PT(Geom) make_square(PN_stdfloat x1, PN_stdfloat y1, PN_stdfloat z1,
		     PN_stdfloat x2, PN_stdfloat y2, PN_stdfloat z2)
{
    auto format = GeomVertexFormat::get_v3n3cpt2();
    auto vdata = new GeomVertexData("square", format, Geom::UH_dynamic);

    GeomVertexWriter vertex(vdata, "vertex");
    GeomVertexWriter normal(vdata, "normal");
    GeomVertexWriter color(vdata, "color");
    GeomVertexWriter texcoord(vdata, "texcoord");

    // make sure we draw the sqaure in the right plane
    if(x1 != x2) {
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

    // adding different colors to the vertex for visibility
    color.add_data4f(1.0, 0.0, 0.0, 1.0);
    color.add_data4f(0.0, 1.0, 0.0, 1.0);
    color.add_data4f(0.0, 0.0, 1.0, 1.0);
    color.add_data4f(1.0, 0.0, 1.0, 1.0);

    texcoord.add_data2f(0.0, 1.0);
    texcoord.add_data2f(0.0, 0.0);
    texcoord.add_data2f(1.0, 0.0);
    texcoord.add_data2f(1.0, 1.0);

    // Quads aren't directly supported by the Geom interface
    // you might be interested in the CardMaker class if you are
    // interested in rectangle though
    PT(GeomTriangles) tris = new GeomTriangles(Geom::UH_dynamic);
    tris->add_vertices(0, 1, 3);
    tris->add_vertices(1, 2, 3);

    auto square = new Geom(vdata);
    square->add_primitive(tris);
    return square;
}

void toggle_tex(void), toggle_lights_side(void), toggle_lights_up(void);

void init(void)
{
    init_osd();
    update_intervals();

    // Note: it isn't particularly efficient to make every face as a separate Geom.
    // instead, it would be better to create one Geom holding all of the faces.
    auto square0 = make_square(-1, -1, -1, 1, -1, 1);
    auto square1 = make_square(-1, 1, -1, 1, 1, 1);
    auto square2 = make_square(-1, 1, 1, 1, -1, 1);
    auto square3 = make_square(-1, 1, -1, 1, -1, -1);
    auto square4 = make_square(-1, -1, -1, -1, 1, 1);
    auto square5 = make_square(1, -1, -1, 1, 1, 1);
    auto snode = new GeomNode("square");
    snode->add_geom(square0);
    snode->add_geom(square1);
    snode->add_geom(square2);
    snode->add_geom(square3);
    snode->add_geom(square4);
    snode->add_geom(square5);

    auto render = window->get_render();
    cube = render.attach_new_node(snode);
    auto cube_hpr = new NPAnim(cube, "cube_hpr", 1.5);
    cube_hpr->set_end_hpr(LPoint3(360, 360, 360));
    cube_hpr->loop();

    // OpenGl by default only draws "front faces" (polygons whose vertices are
    // specified CCW).
    cube.set_two_sided(true);

    test_texture = TexturePool::load_texture("maps/envir-reeds.png");
    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("1", "", EV_FN() { toggle_tex(); }, 0);
    framework.define_key("2", "", EV_FN() { toggle_lights_side(); }, 0);
    framework.define_key("3", "", EV_FN() { toggle_lights_up(); }, 0);

    lights_on = false;
    lights_on1 = false;
    auto slight = new Spotlight("slight");
    slight->set_color(LColor(1, 1, 1, 1));
    auto lens = new PerspectiveLens;
    slight->set_lens(lens);
    slnp = render.attach_new_node(slight);
    slnp1 = render.attach_new_node(slight);
}

void toggle_tex()
{
    if(cube.has_texture())
	cube.set_texture_off(1);
    else
	cube.set_texture(test_texture);
}

void toggle_lights_side()
{
    lights_on = !lights_on;

    auto render = window->get_render();
    if(lights_on) {
	render.set_light(slnp);
	slnp.set_pos(cube, 10, -400, 0);
	slnp.look_at(10, 0, 0);
    } else
	render.set_light_off(slnp);
}

void toggle_lights_up()
{
    lights_on1 = !lights_on1;

    auto render = window->get_render();
    if(lights_on1) {
	render.set_light(slnp1);
	slnp1.set_pos(cube, 10, 0, 400);
	slnp1.look_at(10, 0, 0);
    } else
	render.set_light_off(slnp1);
}
}

int main(int argc, char* argv[])
{
#ifdef SAMPLE_DIR
    get_model_path().prepend_directory(SAMPLE_DIR);
#endif
    if(argc > 1)
	get_model_path().prepend_directory(argv[1]);

    init();

    framework.main_loop();
    framework.close_framework();
    return 0;
}
