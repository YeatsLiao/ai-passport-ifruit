# 工具索引

从项目根目录运行下列命令。普通编译、烧录和截图入口见[项目 README](../README.md)。

## 构建与验证

- [device/](device) 放 Windows 构建与实机截图入口。`device/build.ps1` 默认在 `firmware/build/` 编译，支持 `-IdfRoot`、`-BuildDir`、`-Merge` 和 `-Port`。根目录的 `screenshot.cmd` 调用同一目录中的截图工具。
- [check/](check) 放主机验证和预览。`python -B -m tools.check.test` 检查状态机、存档、WAV 和素材索引。`python -B -m tools.check.preview` 渲染 LVGL 界面，结果在 `.local/preview/`。

## 生成设备资源

- [pack/](pack) 放资源加工脚本。`prepare_content.py` 调用剧情及通讯录处理脚本，输出预制内容与音频序列索引。
- `prepare_ui.py` 与 `prepare_fonts.py` 生成 LVGL 图片和字库。
- `prepare_audio.py` 转换设备音频，`prepare_firmware.py` 校验并汇总到 `firmware/resources/`。
- `python -B -m tools.pack.assets check` 核对[素材索引](../assets/catalog/manifest.json)中的路径、哈希和音频元数据。

## 原件提取与溯源

[extract/](extract) 下的 `gta_*`、`extract_*`、`resolve_*`、`render_*` 负责读取游戏归档、整理映射和渲染声音。Python 模块从项目根目录以 `python -B -m tools.extract.模块名` 运行。本机工具和解包缓存位于 `.local/tooling/` 与 `.local/reference/`。环境、命令和来源见[本地提取工具](../docs/tooling.md)与[游戏资源发现](../docs/game-findings.md)。
