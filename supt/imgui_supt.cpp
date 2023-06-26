/**
 * Portions of this file are copied from pada3d_imgui/sample/main.cpp:
 * 
 * MIT License
 *
 * Copyright (c) 2019 Younguk Kim
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Any other differences are by Thomas J. Moore and are in the Public Domain. */

#include <pandaFramework.h>
#include <pandaSystem.h>
#include <buttonThrower.h>
#include <mouseWatcher.h>
#include <pgTop.h>

#include "imgui_supt.h"
extern "C" {
#include <fontconfig/fontconfig.h>
}

namespace { // prevent global namespace pollution
#include "panda3d_imgui_shaders.h"

void setup_render(Panda3DImGui* panda3d_imgui_helper)
{
    auto task_mgr = AsyncTaskManager::get_global_ptr();

    // NOTE: ig_loop has process_events and 50 sort.
    PT(GenericAsyncTask) new_frame_imgui_task = new GenericAsyncTask("new_frame_imgui", [](GenericAsyncTask*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->new_frame_imgui();
        return AsyncTask::DS_cont;
    }, panda3d_imgui_helper);
    new_frame_imgui_task->set_sort(0);
    task_mgr->add(new_frame_imgui_task);

    PT(GenericAsyncTask) render_imgui_task = new GenericAsyncTask("render_imgui", [](GenericAsyncTask*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->render_imgui();
        return AsyncTask::DS_cont;
    }, panda3d_imgui_helper);
    render_imgui_task->set_sort(40);
    task_mgr->add(render_imgui_task);
}

void setup_button(WindowFramework* window_framework, Panda3DImGui* panda3d_imgui_helper)
{
    if (auto bt = window_framework->get_mouse().find("kb-events"))
    {
        auto ev_handler = EventHandler::get_global_event_handler();

        ButtonThrower* bt_node = DCAST(ButtonThrower, bt.node());
        std::string ev_name;

        ev_name = bt_node->get_button_down_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-button-down";
            bt_node->set_button_down_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            const auto& key_name = ev->get_parameter(0).get_string_value();
            const auto& button = ButtonRegistry::ptr()->get_button(key_name);
            static_cast<Panda3DImGui*>(user_data)->on_button_down_or_up(button, true);
        }, panda3d_imgui_helper);

        ev_name = bt_node->get_button_up_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-button-up";
            bt_node->set_button_up_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            const auto& key_name = ev->get_parameter(0).get_string_value();
            const auto& button = ButtonRegistry::ptr()->get_button(key_name);
            static_cast<Panda3DImGui*>(user_data)->on_button_down_or_up(button, false);
        }, panda3d_imgui_helper);

        ev_name = bt_node->get_keystroke_event();
        if (ev_name.empty())
        {
            ev_name = "imgui-keystroke";
            bt_node->set_keystroke_event(ev_name);
        }
        ev_handler->add_hook(ev_name, [](const Event* ev, void* user_data) {
            wchar_t keycode = ev->get_parameter(0).get_wstring_value()[0];
            static_cast<Panda3DImGui*>(user_data)->on_keystroke(keycode);
        }, panda3d_imgui_helper);
    }
}

void setup_mouse(WindowFramework* window_framework)
{
    window_framework->enable_keyboard();
    auto mouse_watcher = window_framework->get_mouse();
    auto mouse_watcher_node = DCAST(MouseWatcher, mouse_watcher.node());
    DCAST(PGTop, window_framework->get_pixel_2d().node())->set_mouse_watcher(mouse_watcher_node);
}
}

Panda3DImGui *imgui_helper;

void setup_imgui(WindowFramework *framework)
{
    auto window = framework->get_graphics_window();

    // setup Panda3D mouse for pixel2d
    setup_mouse(framework);

    imgui_helper = new Panda3DImGui(window, framework->get_pixel_2d());
    
    // setup ImGUI for Panda3D
    imgui_helper->setup_style();
    imgui_helper->setup_geom();
    // tjm - use baked in shaders to avoid having to look for them
    framework->get_pixel_2d().find("imgui-root").set_shader(
        Shader::make(Shader::SL_GLSL, shader_vert, shader_frag));
    imgui_helper->setup_font();
    imgui_helper->setup_event();
    imgui_helper->on_window_resized();
    imgui_helper->enable_file_drop();

    // setup Panda3D task and key event
    setup_render(imgui_helper);
    setup_button(framework, imgui_helper);

    EventHandler::get_global_event_handler()->add_hook("window-event", [](const Event*, void* user_data) {
        static_cast<Panda3DImGui*>(user_data)->on_window_resized();
    }, imgui_helper);

    // use if the context is in different DLL.
    // FIXME: revisit this if building for Windows, since I have no idea
    // how this works.
    //ImGui::SetCurrentContext(imgui_helper->get_context());
}

using namespace ImGui;
// Font loading support
static ImFont *load_font_file(const char *name, float size)
{
    auto ret = GetIO().Fonts->AddFontFromFileTTF(name, size);
    imgui_helper->setup_font();
    return ret;
}

ImFont *load_font(const char *name, float scale, bool fc_only)
{
    // FIXME:  20 is a random number.
#define fontf() fc_only ? NULL : load_font_file(name, 20);
#if 0
    // This does not fail gracefully.  It assert()s/kills on failure.
    ImFont *ret = fontf();
    if(ret)
	return ret;
#endif
    // Instead, load from file as last resort.
    // I suppose I should at least support a -<size> suffix, but I won't
    auto pat = FcNameParse((const FcChar8 *)name);
    if(!pat)
	return fontf();
    // I have no idea what to do with pat, so I'll just
    // copy what fc-match and xft do:
    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);
    FcResult fcr;
    auto match = FcFontMatch(NULL, pat, &fcr);
    FcPatternDestroy(pat);
    if(!match)
	return fontf();
    FcChar8 *fname;
    if(FcPatternGetString(match, FC_FILE, 0, &fname) != FcResultMatch) {
	FcPatternDestroy(match);
	return fontf(); // can't open font w/o filename
    }
    ImFontConfig cfg; // default constructor
    fcr = FcPatternGetInteger(match, FC_INDEX, 0, &cfg.FontNo);
    double dsize;
    fcr = FcPatternGetDouble(match, FC_PIXEL_SIZE, 0, &dsize);
    if(fcr != FcResultMatch) {
	fcr = FcPatternGetDouble(match, FC_SIZE, 0, &dsize);
	if(fcr != FcResultMatch) {
	    FcPatternDestroy(match);
	    return fontf();
	}
    }
    dsize *= scale;
    ImFont *ret = GetIO().Fonts->AddFontFromFileTTF((const char *)fname, (float)dsize, &cfg /*, glyph_ranges*/);
    //  if(!ret)
    //    return fontf();
    FcPatternDestroy(match);
    imgui_helper->setup_font();
    return ret;
}
