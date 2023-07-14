///////////////////////////////////////////////////////
//                                                   //
// Original code copyright (c) 2019 Ian Eborn.       //
// http://thaumaturge-art.com                        //
// Ported to C++ 2023 by Thomas J. Moore.  All of my //
// work is in the Public Domain, except as required  //
// by compliance with the license on the original    //
// code, stated below.                               //
//                                                   //
// Licensed under the MIT license.                   //
// See "FinalGame/codeLicense".txt, or               //
// https://opensource.org/licenses/MIT               //
//                                                   //
///////////////////////////////////////////////////////

#include <panda3d/pandaFramework.h>
#include <panda3d/auto_bind.h>
#include <panda3d/animBundleNode.h>

PandaFramework framework;

int main(void)
{
    framework.open_framework();
    WindowProperties properties;
    framework.get_default_window_props(properties);
    properties.set_size(1000, 750);
    auto window = framework.open_window(properties, 0);

    auto render = window->get_render();
    auto environment = window->load_model(render, "Models/Misc/environment");

    auto temp_actor = window->load_model(render, "Models/PandaChan/act_p3d_chan");
    /* auto n = */ window->load_model(temp_actor, "Models/PandaChan/a_p3d_chan_run");
    //DCAST(AnimBundleNode, n.get_child(0).node())->get_bundle()->set_name("walk");
    AnimControlCollection anims;
    auto_bind(temp_actor.node(), anims,
	      PartGroup::HMF_ok_anim_extra |
	      PartGroup::HMF_ok_wrong_root_name);
    auto walk = anims.get_anim(0);
    //auto walk = anims.find_anim("walk");
    temp_actor.get_child(0).set_h(180);
    walk->loop(true);
    //anims.loop("walk", true);

    auto camera = window->get_camera_group();
    camera.set_pos(0, 0, 32);
    camera.set_p(-90);

    framework.main_loop();
    framework.close_framework();
    return 0;
}
