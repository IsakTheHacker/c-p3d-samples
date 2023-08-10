#include <panda3d/pandaFramework.h>
#include <panda3d/shader.h>
#include <panda3d/auto_bind.h>

using namespace std;

int main(void)
{
    auto shader = Shader::make(Shader::SL_GLSL,
        // Vertex
        "#version 130\n"
	"\n"
	"in vec4 p3d_Vertex;\n"
	"in vec4 p3d_Color;\n"
	"in vec2 p3d_MultiTexCoord0;\n"
	"\n"
	"in vec4 transform_weight;\n"
	"in uvec4 transform_index;\n"
	"\n"
	"uniform mat4 p3d_ModelViewProjectionMatrix;\n"
	"\n"
	"uniform mat4 p3d_TransformTable[100];\n"
	"\n"
	"out vec4 color;\n"
	"out vec2 texcoord;\n"
	"\n"
	"void main() {\n"
	"  mat4 matrix = p3d_TransformTable[transform_index.x] * transform_weight.x\n"
	"              + p3d_TransformTable[transform_index.y] * transform_weight.y\n"
	"              + p3d_TransformTable[transform_index.z] * transform_weight.z\n"
	"              + p3d_TransformTable[transform_index.w] * transform_weight.w;\n"
	"\n"
	"  gl_Position = p3d_ModelViewProjectionMatrix * matrix * p3d_Vertex;\n"
	"  color = p3d_Color;\n"
	"  texcoord = p3d_MultiTexCoord0;\n"
	"}\n",
	// Fragment
	"#version 130\n"
	"in vec4 color;\n"
	"in vec2 texcoord;\n"
	"\n"
	"uniform sampler2D p3d_Texture0;\n"
	"\n"
	"void main() {\n"
	"  gl_FragColor = color * texture(p3d_Texture0, texcoord);\n"
	"}\n");
    if(shader == nullptr) {
	cerr << "Can't compile shader\n";
	exit(1);
    }

    PandaFramework framework;
    framework.open_framework();
    auto window = framework.open_window();

    const string model = "panda";
    const string anim = "panda-walk";
    PN_stdfloat scale = 1.0;
    PN_stdfloat distance = 6.0;

    // Load the panda model.
    auto panda_actor = window->load_model(window->get_render(), model);
    if(panda_actor.is_empty()) {
	cerr << "Can't load model " << model << '\n';
	exit(1);
    }
    window->load_model(panda_actor, anim);
    AnimControlCollection anims;
    auto_bind(panda_actor.node(), anims);
    if(!anims.get_num_anims()) {
	cerr << "Can't load anims from " << anim << '\n';
	exit(1);
    }
    auto walk = anims.get_anim(0);
    panda_actor.set_scale(scale);
    walk->loop(true);

    // Load the shader to perform the skinning.
    // Also tell Panda that the shader will do the skinning, so
    // that it won't transform the vertices on the CPU.
    CPT(ShaderAttrib) sattr = DCAST(const ShaderAttrib, ShaderAttrib::make(shader));
    auto attr = sattr->set_flag(ShaderAttrib::F_hardware_skinning, true);
    panda_actor.set_attrib(attr);

    // Create a CPU-transformed panda, for reference.
    auto panda_actor2 = window->load_model(window->get_render(), model);
    if(panda_actor2.is_empty()) {
	cerr << "Can't load model " << model << '\n';
	exit(1);
    }
    window->load_model(panda_actor2, anim);
    AnimControlCollection anims2;
    auto_bind(panda_actor2.node(), anims2);
    if(!anims2.get_num_anims()) {
	cerr << "Can't load anims from " << anim << '\n';
	exit(1);
    }
    auto walk2 = anims2.get_anim(0);
    panda_actor2.set_scale(scale);
    walk2->loop(true);

    panda_actor2.set_scale(scale);
    panda_actor2.set_pos(distance, 0, 0);

    window->setup_trackball();
    window->enable_keyboard();
    framework.enable_default_keys();
    // app.trackball.node().setPos(0, 50, -5) (?)
    //window->center_trackball()?  something else?
    window->get_camera_group().set_pos(0, 50, -5);

    framework.main_loop();
    framework.close_framework();
    return 0;
}
