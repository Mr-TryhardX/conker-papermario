"""Install the career lab into Hybrid Heaven's existing native config menu.

Explicit mod operation, not an environment setup step. Idempotent; edits only
the three known integration points and preserves every other source change.
"""
from pathlib import Path
import shutil

root=Path(__file__).resolve().parents[1]
game=root/"native/HybridHeaven"
for name in ["career.hpp","career.cpp"]:
    dest=game/"src/career"/name;dest.parent.mkdir(exist_ok=True)
    shutil.copyfile(root/"src"/name,dest)
for name in ["ui_career.h","ui_career.cpp"]:
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
edit("assets/config_menu.rml","config_menu/career.rml", '\t<link type="text/template" href="config_menu/saves.rml" />', '\t<link type="text/template" href="config_menu/saves.rml" />\n\t<link type="text/template" href="config_menu/career.rml" />')
edit("assets/config_menu.rml",'id="tab_career"', '\t\t\t\t\t</tabset>', '\t\t\t\t\t\t<tab class="tab" id="tab_career"><div>Career Lab</div><div class="tab__indicator"></div></tab>\n\t\t\t\t\t\t<panel class="config" data-model="career_model"><template src="config-menu__career" /></panel>\n\t\t\t\t\t</tabset>')
print("Career Lab installed into native settings. Full rendered build still needs validation.")
