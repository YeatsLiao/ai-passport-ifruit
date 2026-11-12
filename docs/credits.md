# 第三方署名与许可

## 项目原创代码

[MIT 许可证](../LICENSE)仅适用于项目权利人有权授权的原创代码，范围包括 `firmware/CMakeLists.txt`、`firmware/main/`、`tools/`、`tests/` 和 `screenshot.cmd` 中的原创部分。上述路径中使用的第三方内容按各自许可或权利声明处理。

`assets/` 中的游戏素材及其界面截图、`firmware/resources/` 中的设备资源转换产物，以及完整固件镜像整体均在本项目的 MIT 授权范围之外。分发这些内容时应分别核对各组成部分的许可或权利声明。

## 固件组件

- **FoloToy AI Passport** 提供硬件开发资料和 BSP，板级代码位于 `firmware/components/bsp/`。原声明为 Copyright (c) 2026 FoloToy，MIT License。[FoloToy 上游](https://github.com/FoloToy/ai-passport) · [随仓库保留的许可证](../firmware/components/bsp/LICENSE) · [BSP 来源记录](bsp-source.json)
- **Jutta Degener、Carsten Bormann，Technische Universität Berlin** 开发的 GSM 编解码代码用于设备端语音解码。原许可及版权声明保留在[组件 COPYRIGHT](../firmware/components/gsm/COPYRIGHT)。
- **Espressif** 的 ESP-IDF 用于 ESP32-C3 构建，另使用 Button、CMake Utilities、ESP Codec Dev、ESP LVGL Port。依赖版本以[锁文件](../firmware/dependencies.lock)为准，许可见各上游组件随附声明。[ESP-IDF](https://github.com/espressif/esp-idf) · [ESP Component Registry](https://components.espressif.com/)
- **LVGL** 用于固件界面及主机预览，项目依赖 LVGL 9.5.0，采用 MIT。[LVGL 上游](https://github.com/lvgl/lvgl) · [LVGL 许可说明](https://lvgl.io/docs/open/9.0/intro/)

## 游戏素材与界面参考

- **Rockstar Games / Take-Two Interactive** 的《Grand Theft Auto V》PC Legacy 版是游戏 UI、头像、插画、字体、对白及音效的主要来源。设备资源和界面预览包含其中的转换或呈现结果，具体游戏归档路径、哈希和权利状态见[逐项素材清单](../assets/catalog/manifest.json)及[来源记录](sources.json)。
- **iFruit 网页模拟器**用于界面旁参考。来源记录标注素材未从该站导入。[网页模拟器](https://ifruit-sim.yifubei.top) · [参考记录](sources.json)
- **AI Passport 希卡之石项目**提供截图工具入口及仓库文档结构的参考。[希卡之石项目](https://github.com/YeatsLiao/ai-passport-sheikah-slate)
- **Vice City 彩蛋海报**由用户提供，作品及其中游戏元素的相关权利归各自权利人。[海报来源条目](../assets/catalog/easteregg.json)
- **洛圣都来电封面**是为本项目生成的同人插画，描绘《GTA V》的三位主角和 iFruit 手机。相关游戏角色及标识的权利归各自权利人。[封面文件](../assets/concepts/los-santos-calling.png)

## 开发工具与技术参考

- **dexyfex / CodeWalker** 的 AWC、REL 格式实现与合成器用于本地分析和离线声音重建。[CodeWalker](https://github.com/dexyfex/CodeWalker)
- **gizzdev / GTAUtil 与 RageLib** 用于读取游戏归档，**vgmstream** 用于音频流解码。[GTAUtil](https://github.com/gizzdev/gtautil) · [vgmstream](https://github.com/vgmstream/vgmstream)
- **JPEXS FFDec** 与 **GTA V Script Decompiler** 用于检查界面与脚本，**REA** 用于网页参考检查。工具版本、用途及本地路径见[提取工具说明](tooling.md)。[JPEXS FFDec](https://github.com/jindrapetrik/jpexs-decompiler) · [GTA V Script Decompiler](https://github.com/maybegreat48/GTA-V-Script-Decompiler)
