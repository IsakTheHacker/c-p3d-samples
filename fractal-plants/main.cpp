/*
 *  Translation of Python fractal-plants sample from Panda3D.
 *  https://github.com/panda3d/panda3d/tree/v1.10.13/samples/fractal-plants
 *
 * Original C++ conversion by Thomas J. Moore July 2023.
 *
 * Comments are mostly extracted from the Python sample, such as:
 *
 * Author: Kwasi Mensah (kmensah@andrew.cmu.edu)
 * Date: 8/05/2005
 *
 * This demo shows how to make quasi-fractal trees in Panda.
 * Its primarily meant to be a more complex example on how to use
 * Panda's Geom interface.
 */
#include <panda3d/pandaFramework.h>
#include <panda3d/ambientLight.h>
#include <panda3d/spotlight.h>
#include <panda3d/geomTristrips.h>
#include <panda3d/cullFaceAttrib.h>
#include <panda3d/texturePool.h>

#include "sample_supt.h"

namespace { // don't export/pollute the global namespace
// Global variables
PandaFramework framework;
WindowFramework *window;
std::string sample_path
#ifdef SAMPLE_DIR
    = SAMPLE_DIR "/"
#endif
    ;
Randomizer rands;
unsigned num_primitives = 0;

// TJM: Note that this was somewhat restuctured to place code in init() and
// non-code outside of init(), rather than intermixing them randomly.

void setup_lights(void), setup_tapper(void);

// TJM: like other samples, this is the common code to add instruction text
TextNode *add_instruction(const std::string &msg, int off)
{
    auto a2d = window->get_aspect_2d();
    auto text_node = new TextNode("instruction");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text(msg);
    text_node->set_align(TextNode::A_left);
    // style = 1 - default
    text_node->set_text_color(1, 1, 1, 1);
    text.set_pos(-1.0/a2d.get_sx() + 0.06, 0, 1.0 - 0.10 - off * 0.06); // TopLeft == (-1,0,1)
    text.set_scale(0.05);
    return text_node; // may_change = true
}

void init(void)
{
    framework.open_framework();
    framework.set_window_title("Fractal Tree - C++ Panda3D Samples");
    window = framework.open_window();

    window->get_camera_group().set_pos(0, -180, 30);

    // There is no convenient "OnScreenText" class, although one could
    // be written.  Instead, here are the manual steps:
    auto a2d = window->get_aspect_2d();
    TextNode *text_node = new TextNode("title");
    auto text = a2d.attach_new_node(text_node);
    text_node->set_text("Panda3D: Tutorial - Procedurally Making a Tree");
    // style = 1 - default
    text_node->set_text_color(1, 1, 1, 1);
    text_node->set_align(TextNode::A_center); // "default"
    text.set_pos(0, 0, -1+0.1); // BottomCenter == (0,0,-1)
    text.set_scale(0.08);

    add_instruction("Q: Start Scene Over", 0);
    add_instruction("W: Add Another Tree", 1);

    setup_lights();
    setup_tapper();
}

// this computes the new Axis which we'll make a branch grow alowng when we
// split

struct vec_list_t {
    LVector3 fwd, perp1, perp2;
};

vec_list_t random_axis(const vec_list_t &vec_list)
{
    const auto &fwd = vec_list.fwd, &perp1 = vec_list.perp1, &perp2 = vec_list.perp2;

    auto nfwd = fwd + perp1 * (2 * rands.random_real(1) - 1) +
                      perp2 * (2 * rands.random_real(1) - 1);
    nfwd.normalize();

    auto nperp2 = nfwd.cross(perp1);
    nperp2.normalize();

    auto nperp1 = nfwd.cross(nperp2);
    nperp1.normalize();

    return {nfwd, nperp1, nperp2};
}

// this makes smalle variations in direction when we are growing a branch
// but not splitting
vec_list_t small_random_axis(const vec_list_t &vec_list)
{
    const auto &fwd = vec_list.fwd, &perp1 = vec_list.perp1, &perp2 = vec_list.perp2;

    auto nfwd = fwd + perp1 * (1 * rands.random_real(1) - 0.5) +
                      perp2 * (1 * rands.random_real(1) - 0.5);
    nfwd.normalize();

    auto nperp2 = nfwd.cross(perp1);
    nperp2.normalize();

    auto nperp1 = nfwd.cross(nperp2);
    nperp1.normalize();

    return {nfwd, nperp1, nperp2};
}

// this draws the body of the tree. This draws a ring of vertices and connects the rings with
// triangles to form the body.
// this keep_drawing paramter tells the function wheter or not we're at an end
// if the vertices before you were an end, dont draw branches to it
void draw_body(NodePath &node_path, GeomVertexData *vdata, const LVector3 &pos,
	       const vec_list_t &vec_list, PN_stdfloat radius=1,
	       bool keep_drawing=true, unsigned num_vertices=8)
{
    PT(Geom) circle_geom = new Geom(vdata);

    GeomVertexWriter vert_writer(vdata, "vertex");
    GeomVertexWriter color_writer(vdata, "color");
    GeomVertexWriter normal_writer(vdata, "normal");
    GeomVertexRewriter draw_re_writer(vdata, "drawFlag");
    GeomVertexRewriter tex_re_writer(vdata, "texcoord");

    auto start_row = vdata->get_num_rows();
    vert_writer.set_row(start_row);
    color_writer.set_row(start_row);
    normal_writer.set_row(start_row);

    PN_stdfloat s_coord = 0;

    if(start_row != 0) {
        tex_re_writer.set_row(start_row - num_vertices);
        s_coord = tex_re_writer.get_data2f().get_x() + 1;

        draw_re_writer.set_row(start_row - num_vertices);
        if(draw_re_writer.get_data1f() == false)
            s_coord -= 1;
    }

    draw_re_writer.set_row(start_row);
    tex_re_writer.set_row(start_row);

    auto angle_slice = 2 * M_PI / num_vertices;
    double curr_angle = 0;

    //auto axis_adj=LMatrix4.rotate_mat(45, axis)*LMatrix4.scale_mat(radius)*LMatrix4.translate_mat(pos);

    const auto &perp1 = vec_list.perp1;
    const auto &perp2 = vec_list.perp2;

    // vertex information is written here
    for(unsigned i = 0; i < num_vertices; i++) {
        auto adj_circle = pos + 
                 (perp1 * cos(curr_angle) + perp2 * sin(curr_angle)) *
                 radius;
        auto normal = perp1 * cos(curr_angle) + perp2 * sin(curr_angle);
	normal_writer.add_data3f(normal);
        vert_writer.add_data3f(adj_circle);
        tex_re_writer.add_data2f(s_coord, (i + 0.001) / (num_vertices - 1));
        color_writer.add_data4f(0.5, 0.5, 0.5, 1);
        draw_re_writer.add_data1f(keep_drawing);
        curr_angle += angle_slice;
    }

    if(start_row == 0)
        return;

    GeomVertexReader draw_reader(vdata, "drawFlag");
    draw_reader.set_row(start_row - num_vertices);

    // we cant draw quads directly so we use Tristrips
    if(draw_reader.get_data1i() != 0) {
        PT(GeomTristrips) lines = new GeomTristrips(Geom::UH_static);
        unsigned half = num_vertices * 0.5;
        for(unsigned i = 0; i < num_vertices; i++) {
            lines->add_vertex(i + start_row);
            if(i < half)
                lines->add_vertex(i + start_row - half);
            else
                lines->add_vertex(i + start_row - half - num_vertices);
	}

        lines->add_vertex(start_row);
        lines->add_vertex(start_row - half);
        lines->close_primitive();
        lines->decompose();
        circle_geom->add_primitive(lines);

        auto circle_geom_node = new GeomNode("Debug");
        circle_geom_node->add_geom(circle_geom);

        // I accidentally made the front-face face inwards. Make reverse makes the tree render properly and
        // should cause any surprises to any poor programmer that tries to use
        // this code
        circle_geom_node->set_attrib(CullFaceAttrib::make_reverse(), 1);
        num_primitives += num_vertices * 2;

        node_path.attach_new_node(circle_geom_node);
    }
}

// this draws leafs when we reach an end
void draw_leaf(NodePath &node_path, LVector3 pos=LVector3(0, 0, 0),
	       const vec_list_t &vec_list={{0, 0, 1}, {1, 0, 0}, {0, -1, 0}},
	       PN_stdfloat scale=0.125)
{
    // use the vectors that describe the direction the branch grows to make the right
    // rotation matrix
    LMatrix4 new_cs(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    new_cs.set_row(0, vec_list.perp2);  // right
    new_cs.set_row(1, vec_list.perp1);  // up
    new_cs.set_row(2, vec_list.fwd);    // forward
    new_cs.set_row(3, LVector3(0, 0, 0));
    new_cs.set_col(3, LColor(0, 0, 0, 1));

    auto axis_adj = LMatrix4::scale_mat(scale) * new_cs * LMatrix4::translate_mat(pos);

    // orginlly made the leaf out of geometry but that didnt look good
    // I also think there should be a better way to handle the leaf texture other than
    // hardcoding the filename
    // [tjm: this means the original "vdata" parameter is not used]
    auto leaf_model = def_load_model("models/shrubbery");
    auto leaf_texture = def_load_texture("models/material-10-cl.png");

    leaf_model.reparent_to(node_path);
    leaf_model.set_texture(leaf_texture, 1);
    leaf_model.set_transform(TransformState::make_mat(axis_adj));
}

// recursive algorthim to make the tree
void make_fractal_tree(GeomVertexData *bodydata, NodePath &node_path,
		       LVector3 length, const LVector3 &pos={0, 0, 0},
		       unsigned num_iterations=11,
		       unsigned num_copies=4,
		       vec_list_t vec_list={{0, 0, 1}, {1, 0, 0}, {0, -1, 0}})
{
    if(num_iterations > 0) {

        draw_body(node_path, bodydata, pos, vec_list, length.get_x());

        // move foward along the right axis
        auto new_pos = pos + vec_list.fwd * length.length();

        // only branch every third level (sorta)
        if(num_iterations % 3 == 0) {
            // decrease dimensions when we branch
            length = LVector3(
                length.get_x() / 2, length.get_y() / 2, length.get_z() / 1.1);
            for(unsigned i = 0; i < num_copies; i++)
                make_fractal_tree(bodydata, node_path, length, new_pos,
				  num_iterations - 1, num_copies,
				  random_axis(vec_list));
        } else
            // just make another branch connected to this one with a small
            // variation in direction
            make_fractal_tree(bodydata, node_path, length, new_pos,
			      num_iterations - 1, num_copies,
			      small_random_axis(vec_list));

    } else {
        draw_body(node_path, bodydata, pos, vec_list, length.get_x(), false);
        draw_leaf(node_path, pos, vec_list);
    }
}

NodePath slnp;
AsyncTask::DoneStatus update_light(GenericAsyncTask *task, void *);

void setup_lights()
{
    auto render = window->get_render();
    auto alight = new AmbientLight("alight");
    alight->set_color({0.5, 0.5, 0.5, 1});
    auto alnp = render.attach_new_node(alight);
    render.set_light(alnp);

    auto slight = new Spotlight("slight");
    slight->set_color({1, 1, 1, 1});
    auto lens = new PerspectiveLens;
    slight->set_lens(lens);
    slnp = render.attach_new_node(slight);
    render.set_light(slnp);

    slnp.set_pos(0, 0, 40);

    framework.get_task_mgr().add(new GenericAsyncTask("rotating Light", update_light, 0));
}

// rotating light to show that normals are calculated correctly
AsyncTask::DoneStatus update_light(GenericAsyncTask *task, void *)
{
    auto time = task->get_elapsed_time();
    auto curr_pos = slnp.get_pos();
    curr_pos.set_x(100 * cos(time) / 2);
    curr_pos.set_y(100 * sin(time) / 2);
    slnp.set_pos(curr_pos);

    slnp.look_at(window->get_render());
    return AsyncTask::DS_cont;
}

CPT(GeomVertexFormat) format;
PT(Texture) bark_texture;
int num_iterations = 11, num_copies = 4;
TextNode *up_down_event, *left_right_event;
// TJM: merged inc/dec functions to single adj to avoid duplication
void adj_iterations(int), adj_copies(int), regen_tree(void), add_tree(void);

// add some interactivity to the program
void setup_tapper()
{
    auto format_array = new GeomVertexArrayFormat;
    format_array->add_column(
        InternalName::make("drawFlag"), 1, Geom::NT_uint8, Geom::C_other);

    auto fmt = new GeomVertexFormat(*GeomVertexFormat::get_v3n3cpt2());
    fmt->add_array(format_array);
    format = GeomVertexFormat::register_format(fmt);

    PT(GeomVertexData) bodydata = new GeomVertexData("body vertices", format, Geom::UH_static);

    bark_texture = def_load_texture("barkTexture.jpg");
    NodePath tree_node_path("Tree Holder");
    make_fractal_tree(bodydata, tree_node_path, LVector3(4, 4, 7));

    tree_node_path.set_texture(bark_texture, 1);
    tree_node_path.reparent_to(window->get_render());

    window->enable_keyboard();
    framework.define_key("escape", "", framework.event_esc, &framework);
    framework.define_key("q", "", EV_FN() { regen_tree(); }, 0);
    framework.define_key("w", "", EV_FN() { add_tree(); }, 0);
    framework.define_key("arrow_up", "", EV_FN() { adj_iterations(1); }, 0);
    framework.define_key("arrow_down", "", EV_FN() { adj_iterations(-1); }, 0);
    framework.define_key("arrow_right", "", EV_FN() { adj_copies(1); }, 0);
    framework.define_key("arrow_left", "", EV_FN() { adj_copies(-1); }, 0);

    up_down_event = add_instruction("", 2);
    adj_iterations(0);
    left_right_event = add_instruction("", 3);
    adj_copies(0);
}

void adj_iterations(int amt)
{
    if(num_iterations > 0 || amt >= 0) // tjm: don't dec too much
	num_iterations += amt;
    up_down_event->set_text("Up/Down: Increase/Decrease the number of iterations (" +
			    std::to_string(num_iterations) + ')');
}

void adj_copies(int amt)
{
    if(num_copies > 0 || amt >= 0) // tjm: don't dec too miuch
	num_copies += amt;
    left_right_event->set_text("Left/Right: Increase/Decrease branching (" +
			       std::to_string(num_copies) + ')');
}

void regen_tree()
{
    auto render = window->get_render();
    auto forest = render.find_all_matches("Tree Holder");
    forest.detach();

    PT(GeomVertexData) bodydata = new GeomVertexData("body vertices", format, Geom::UH_static);

    NodePath tree_node_path("Tree Holder");
    make_fractal_tree(bodydata, tree_node_path, LVector3(4, 4, 7),
		      LVector3(0, 0, 0), num_iterations, num_copies);

    tree_node_path.set_texture(bark_texture, 1);
    tree_node_path.reparent_to(render);
}

void add_tree()
{
    PT(GeomVertexData) bodydata = new GeomVertexData("body vertices", format, Geom::UH_static);

    LVector3 random_place(
            200 * rands.random_real(1) - 100, 200 * rands.random_real(1) - 100, 0);
    // random_place.normalize();

    NodePath tree_node_path("Tree Holder");
    make_fractal_tree(bodydata, tree_node_path, LVector3(4, 4, 7),
		      random_place, num_iterations, num_copies);

    tree_node_path.set_texture(bark_texture, 1);
    tree_node_path.reparent_to(window->get_render());
}
}

int main(int argc, char* argv[]) {
    if(argc > 1)
	sample_path = argv[1]; // old C++ sample had -vs for ../../.., but I don't care

    init();
    std::cout << num_primitives << '\n';

    //Do the main loop (start 3d rendering and event processing)
    framework.main_loop();
    framework.close_framework();
    return 0;
}
