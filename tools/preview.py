"""Render actual LVGL firmware UI on the host; no flashed-device claim."""
from pathlib import Path
import os,shutil,subprocess
from PIL import Image
ROOT=Path(__file__).resolve().parents[1];folder=ROOT/'build/preview';folder.mkdir(parents=True,exist_ok=True)
subprocess.run([os.sys.executable,str(ROOT/'tools/prepare_content.py')],check=True)
subprocess.run([os.sys.executable,str(ROOT/'tools/prepare_ui.py')],check=True)
(folder/'lv_conf.h').write_text('#ifndef LV_CONF_H\n#define LV_CONF_H\n#define LV_COLOR_DEPTH 16\n#define LV_USE_OS 0\n#define LV_USE_STDLIB_MALLOC 1\n#define LV_USE_STDLIB_STRING 1\n#define LV_USE_STDLIB_SPRINTF 1\n#define LV_USE_LOG 0\n#define LV_FONT_MONTSERRAT_14 1\n#define LV_FONT_MONTSERRAT_20 1\n#endif\n')
def q(p):return '"'+Path(p).as_posix()+'"'
cmake=f"cmake_minimum_required(VERSION 3.20)\nproject(ifruit_preview C)\nset(CMAKE_C_STANDARD 11)\nfile(GLOB_RECURSE LV_SOURCES {q(ROOT/'managed_components/lvgl__lvgl/src/*.c')})\nadd_executable(preview ${{LV_SOURCES}} {q(ROOT/'main/ui.c')} {q(ROOT/'main/phone.c')} {q(ROOT/'main/world.c')} {q(ROOT/'build/generated/phone_content.c')} {q(ROOT/'tools/preview.c')} {q(ROOT/'build/generated/ui_assets.c')})\ntarget_include_directories(preview PRIVATE {q(folder)} {q(ROOT/'main')} {q(ROOT/'build/generated')} {q(ROOT/'managed_components/lvgl__lvgl')})\ntarget_compile_definitions(preview PRIVATE LV_CONF_INCLUDE_SIMPLE LV_KCONFIG_IGNORE)\ntarget_link_libraries(preview PRIVATE m)\n"
(folder/'CMakeLists.txt').write_text(cmake)
cc=shutil.which('gcc');env=dict(os.environ);env['PATH']=str(Path(cc).parent)+os.pathsep+env['PATH']
subprocess.run(['cmake','-S',str(folder),'-B',str(folder/'out'),'-G','Ninja','-DCMAKE_C_COMPILER='+cc,'-DCMAKE_BUILD_TYPE=Release'],check=True,env=env)
subprocess.run(['cmake','--build',str(folder/'out'),'--parallel','8'],check=True,env=env)
subprocess.run([str(folder/'out/preview.exe'),str(folder)],check=True,env=env)
for p in folder.glob('*.ppm'):Image.open(p).save(p.with_suffix('.png'))
print('Rendered actual LVGL UI to',folder)
