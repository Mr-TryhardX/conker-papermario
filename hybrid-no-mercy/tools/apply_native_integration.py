"""Install the career lab into Hybrid Heaven's existing native config menu.

Explicit mod operation, not an environment setup step. Idempotent; edits only
the known integration points and preserves every other source change.
"""
from pathlib import Path
import shutil

root=Path(__file__).resolve().parents[1]
game=root/"native/HybridHeaven"
for name in ["career.hpp","career.cpp"]:
    dest=game/"src/career"/name;dest.parent.mkdir(exist_ok=True)
    shutil.copyfile(root/"src"/name,dest)
for name in ["ui_career.h","ui_career.cpp","ui_nomercy_preview.h","ui_nomercy_preview.cpp","nomercy_audio.h","nomercy_audio.cpp"]:
    shutil.copyfile(root/"integration"/name,game/"src/ui"/name)
shutil.copyfile(root/"integration/career.rml",game/"assets/config_menu/career.rml")

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
    if (root/source).exists():shutil.copyfile(root/source,audio/name)
    else:print("Missing captured music:",source)
bank=root/"asset-import/nomercy/audio/bank1"
if bank.exists():shutil.copytree(bank,audio/"bank1",dirs_exist_ok=True)
print("Career Lab and No Mercy import bench installed. Rebuild and verify actual rendering/audio after changes.")
