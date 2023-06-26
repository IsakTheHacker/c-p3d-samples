#include <imgui.h>
#include <imgui_stdlib.h>

//#include <ImGuiFileDialog.h>

//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wunused-variable"
//#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
//#ifndef __clang__ // clang just warns that this warning group doesn't exist
//#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
//#endif
//#include "imgui_markdown.h"
//#pragma GCC diagnostic pop

#include <panda3d_imgui.hpp>

void setup_imgui(WindowFramework *);
ImFont *load_font(const char *name, float scale = 1.0f, bool fc_only = false);
#define imgui_draw(f) \
    EventHandler::get_global_event_handler()->add_hook(Panda3DImGui::NEW_FRAME_EVENT_NAME, [](const Event*) { f; })
