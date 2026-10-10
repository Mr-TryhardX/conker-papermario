"""Install the career lab into Hybrid Heaven's existing native config menu.

Explicit mod operation, not an environment setup step. Idempotent; edits only
the known integration points and preserves every other source change.
"""
from pathlib import Path
import shutil

def copy_if_changed(source,destination):
    source,destination=Path(source),Path(destination)
    if destination.exists() and destination.read_bytes()==source.read_bytes():return
    shutil.copyfile(source,destination)

def write_if_changed(path,text):
    if path.exists() and path.read_text()==text:return
    path.write_text(text)

root=Path(__file__).resolve().parents[1]
game=root/"native/HybridHeaven"
for name in ["career.hpp","career.cpp"]:
    dest=game/"src/career"/name;dest.parent.mkdir(exist_ok=True)
    copy_if_changed(root/"src"/name,dest)
for name in ["ui_career.h","ui_career.cpp","ui_nomercy_preview.h","ui_nomercy_preview.cpp","nomercy_audio.h","nomercy_audio.cpp","ui_nomercy_actor.h","ui_nomercy_actor.cpp"]:
    copy_if_changed(root/"integration"/name,game/"src/ui"/name)
copy_if_changed(root/"integration/career.rml",game/"assets/config_menu/career.rml")

def edit(relative,marker,old,new):
    path=game/relative;source=path.read_text()
    if marker in source:return
    if source.count(old)!=1:raise RuntimeError(f"Integration point changed: {relative}")
    path.write_text(source.replace(old,new,1))

edit("src/ui/ui_config.cpp",'#include "ui_career.h"', '#include "ui_saves.h"', '#include "ui_saves.h"\n#include "ui_career.h"')
edit("src/ui/ui_config.cpp","recompui::make_career_bindings(context);", "        recompui::make_saves_bindings(context);", "        recompui::make_saves_bindings(context);\n        recompui::make_career_bindings(context);")
edit("CMakeLists.txt","src/ui/ui_career.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_saves.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_saves.cpp\n    ${CMAKE_SOURCE_DIR}/src/ui/ui_career.cpp\n    ${CMAKE_SOURCE_DIR}/src/career/career.cpp")
edit("CMakeLists.txt","thirdparty/json)", "target_include_directories(HybridHeavenRecompiled PRIVATE ${HH_RECOMP_INCLUDES})", "target_include_directories(HybridHeavenRecompiled PRIVATE ${HH_RECOMP_INCLUDES})\ntarget_include_directories(HybridHeavenRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/lib/N64ModernRuntime/thirdparty/json)")
edit("src/main/rt64_render_context.cpp","Respect portable mode for RT64", "    appConfig.useConfigurationFile = false;", "    appConfig.useConfigurationFile = false;\n    // Respect portable mode for RT64's own log folder as well as game saves.\n    if (std::filesystem::exists(\"portable.txt\")) {\n        appConfig.detectDataPath = false;\n        appConfig.dataPath = std::filesystem::current_path() / \".rt64\";\n    }")
edit("assets/config_menu.rml","config_menu/career.rml", '\t<link type="text/template" href="config_menu/saves.rml" />', '\t<link type="text/template" href="config_menu/saves.rml" />\n\t<link type="text/template" href="config_menu/career.rml" />')
edit("assets/config_menu.rml",'id="tab_career"', '\t\t\t\t\t</tabset>', '\t\t\t\t\t\t<tab class="tab" id="tab_career"><div>Career Lab</div><div class="tab__indicator"></div></tab>\n\t\t\t\t\t\t<panel class="config" data-model="career_model"><template src="config-menu__career" /></panel>\n\t\t\t\t\t</tabset>')
edit("CMakeLists.txt","src/ui/ui_nomercy_preview.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_career.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_career.cpp\n    ${CMAKE_SOURCE_DIR}/src/ui/ui_nomercy_preview.cpp\n    ${CMAKE_SOURCE_DIR}/src/ui/nomercy_audio.cpp")
edit("src/ui/ui_elements.cpp", '#include "ui_nomercy_preview.h"', '#include "ui_elements.h"', '#include "ui_elements.h"\n#include "ui_nomercy_preview.h"')
edit("src/ui/ui_elements.cpp", 'CUSTOM_ELEMENT("nomercy-preview"', 'static RecompCustomElement custom_elements[] = {', 'static RecompCustomElement custom_elements[] = {\n    CUSTOM_ELEMENT("nomercy-preview", recompui::ElementNoMercyPreview),')
edit("src/ui/ui_config.cpp", "recompui::register_nomercy_images();", '\t\tconfig_context = recompui::create_context(hybridheaven::get_asset_path("config_menu.rml"));', '\t\trecompui::register_nomercy_images();\n\t\tconfig_context = recompui::create_context(hybridheaven::get_asset_path("config_menu.rml"));')
converted=root/"asset-import/converted"
if converted.exists():
    shutil.copytree(converted,game/"assets/nomercy",dirs_exist_ok=True)
audio=game/"assets/nomercy/audio"
audio.mkdir(parents=True,exist_ok=True)
for name,source in [("shop.wav","assets/audio/no-mercy-shop.wav"),("Kane.wav","assets/audio/entrances/Kane.wav"),("The-Rock.wav","assets/audio/entrances/The-Rock.wav")]:
    if (root/source).exists():copy_if_changed(root/source,audio/name)
    else:print("Missing captured music:",source)
bank=root/"asset-import/nomercy/audio/bank1"
if bank.exists():shutil.copytree(bank,audio/"bank1",dirs_exist_ok=True)
print("Career Lab and No Mercy import bench installed. Rebuild and verify actual rendering/audio after changes.")

edit("CMakeLists.txt", "src/ui/ui_nomercy_actor.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_nomercy_preview.cpp", "    ${CMAKE_SOURCE_DIR}/src/ui/ui_nomercy_preview.cpp\n    ${CMAKE_SOURCE_DIR}/src/ui/ui_nomercy_actor.cpp")
edit("src/ui/ui_elements.cpp", '#include "ui_nomercy_actor.h"', '#include "ui_nomercy_preview.h"', '#include "ui_nomercy_preview.h"\n#include "ui_nomercy_actor.h"')
edit("src/ui/ui_elements.cpp", 'CUSTOM_ELEMENT("nomercy-actor"', 'static RecompCustomElement custom_elements[] = {', 'static RecompCustomElement custom_elements[] = {\n    CUSTOM_ELEMENT("nomercy-actor", recompui::ElementNoMercyActor),')
actor=root/"asset-import/kane-native"
if actor.exists():shutil.copytree(actor,game/"assets/nomercy/actor",dirs_exist_ok=True)

copy_if_changed(root/"integration/hh_player_probe.hpp",game/"src/main/hh_player_probe.hpp")
edit("src/main/rt64_render_context.cpp", '#include "hh_player_probe.hpp"', '#include <algorithm>', '#include <algorithm>\n#include "hh_player_probe.hpp"')
edit("src/main/rt64_render_context.cpp", 'hh_player_probe::tick(app->core.RDRAM);', '    frame_probe.frame_start(hh_overlay_load_count());', '    frame_probe.frame_start(hh_overlay_load_count());\n    hh_player_probe::tick(app->core.RDRAM);')

# Preserve the original emitter, generated from the user's local game, under a
# second name. The opt-in wrapper can observe it without copying original code
# into the delivery repository or guessing its behavior.
original=game/"RecompiledFuncs/funcs_47.c"
source=original.read_text()
start=source.index("RECOMP_FUNC void func_8000C768_D368(")
end=source.index("\nRECOMP_FUNC ",start+1)
body=source[start:end].replace("RECOMP_FUNC void func_8000C768_D368(","static void hh_original_actor_draw(",1)
write_if_changed(game/"src/game/hh_actor_draw_original.inc",body)
copy_if_changed(root/"integration/hh_actor_draw.c",game/"src/game/hh_actor_draw.c")
edit("CMakeLists.txt", "src/game/hh_actor_draw.c", "target_include_directories(HybridHeavenRecompiled PRIVATE ${HH_RECOMP_INCLUDES})", "target_include_directories(HybridHeavenRecompiled PRIVATE ${HH_RECOMP_INCLUDES})\ntarget_sources(HybridHeavenRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/src/game/hh_actor_draw.c)")

copy_if_changed(root/"integration/hh_kane_model.cpp",game/"src/game/hh_kane_model.cpp")
copy_if_changed(root/"integration/hh_kane_motion.hpp",game/"src/game/hh_kane_motion.hpp")
edit("CMakeLists.txt", "src/game/hh_kane_model.cpp", "target_sources(HybridHeavenRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/src/game/hh_actor_draw.c)", "target_sources(HybridHeavenRecompiled PRIVATE ${CMAKE_SOURCE_DIR}/src/game/hh_actor_draw.c ${CMAKE_SOURCE_DIR}/src/game/hh_kane_model.cpp)")

# Remove only our earlier experimental whole-frame mode: original commands
# can contain hardware-masked addresses that must keep their normal semantics.
p=game/"src/main/rt64_render_context.cpp"
s=p.read_text();s=s.replace('\n    // Kane model heap needs RT64 extended RDRAM; keep the default path unchanged.\n    if (std::getenv("HH_KANE_IN_GAME") != nullptr) app->state->setExtendedRDRAM(true);','')
write_if_changed(p,s)
