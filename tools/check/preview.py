"""Render actual LVGL firmware UI on the host; no flashed-device claim."""
from pathlib import Path
import os,shutil,subprocess
from PIL import Image
ROOT=Path(__file__).resolve().parents[2];folder=ROOT/'.local/preview';folder.mkdir(parents=True,exist_ok=True)
subprocess.run([os.sys.executable,'-B','-m','tools.pack.prepare_content'],check=True)
subprocess.run([os.sys.executable,'-B','-m','tools.pack.prepare_ui'],check=True)
def write_if_changed(path,value):
 if not path.exists() or path.read_text()!=value:path.write_text(value)

write_if_changed(folder/'lv_conf.h','#ifndef LV_CONF_H\n#define LV_CONF_H\n#include <stdlib.h>\n#define LV_COLOR_DEPTH 16\n#define LV_USE_OS 0\n#define LV_USE_STDLIB_MALLOC 0\n#define LV_MEM_SIZE (32 * 1024U)\n#define LV_MEM_POOL_EXPAND_SIZE 0\n#define LV_CACHE_DEF_SIZE 0\n#define LV_IMAGE_HEADER_CACHE_DEF_CNT 0\n#define LV_ASSERT_HANDLER_INCLUDE <stdio.h>\n#define LV_ASSERT_HANDLER fprintf(stderr,"LVGL assertion %s:%d\\n",__FILE__,__LINE__);fflush(stderr);abort();\n#define LV_USE_STDLIB_STRING 1\n#define LV_USE_STDLIB_SPRINTF 1\n#define LV_USE_LOG 0\n#define LV_FONT_MONTSERRAT_14 1\n#define LV_FONT_MONTSERRAT_20 1\n#endif\n')
def q(p):return '"'+Path(p).as_posix()+'"'
cmake=f"cmake_minimum_required(VERSION 3.20)\nproject(ifruit_preview C)\nset(CMAKE_C_STANDARD 11)\nfile(GLOB_RECURSE LV_SOURCES {q(ROOT/'firmware/managed_components/lvgl__lvgl/src/*.c')})\nadd_executable(preview ${{LV_SOURCES}} {q(ROOT/'firmware/main/ui.c')} {q(ROOT/'firmware/main/phone.c')} {q(ROOT/'firmware/main/communications.c')} {q(ROOT/'firmware/main/world.c')} {q(ROOT/'firmware/main/simulation.c')} {q(ROOT/'firmware/main/story.c')} {q(ROOT/'firmware/main/phonebook.c')} {q(ROOT/'.local/generated/phone_content.c')} {q(ROOT/'tools/check/preview.c')} {q(ROOT/'.local/generated/ui_assets.c')} {q(ROOT/'.local/generated/ui_fonts.c')})\ntarget_include_directories(preview PRIVATE {q(folder)} {q(ROOT/'firmware/main')} {q(ROOT/'.local/generated')} {q(ROOT/'firmware/managed_components/lvgl__lvgl')})\ntarget_compile_definitions(preview PRIVATE LV_CONF_INCLUDE_SIMPLE LV_KCONFIG_IGNORE)\ntarget_link_libraries(preview PRIVATE m)\n"
write_if_changed(folder/'CMakeLists.txt',cmake)
cc=shutil.which('gcc');env=dict(os.environ);env['PATH']=str(Path(cc).parent)+os.pathsep+env['PATH']
subprocess.run(['cmake','-S',str(folder),'-B',str(folder/'out'),'-G','Ninja','-DCMAKE_C_COMPILER='+cc,'-DCMAKE_BUILD_TYPE=Release'],check=True,env=env)
subprocess.run(['cmake','--build',str(folder/'out'),'--parallel','8'],check=True,env=env)
subprocess.run([str(folder/'out/preview.exe'),str(folder)],check=True,env=env)
for p in folder.glob('*.ppm'):Image.open(p).save(p.with_suffix('.png'))
motion=[Image.open(p).convert('RGB') for p in sorted(folder.glob('native_motion_*.png'))]
if motion:motion[0].save(folder/'native_home_motion.gif',save_all=True,append_images=motion[1:],duration=[20]*(len(motion)-1)+[700],loop=0)
print('Rendered actual LVGL UI to',folder)
