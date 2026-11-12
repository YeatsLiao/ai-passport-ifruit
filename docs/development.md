# 开发与素材更新

所有命令从项目根目录执行。日常编译、烧录和截图见[项目 README](../README.md)。ESP-IDF 5.5.5 已用于当前构建，组件的版本范围见 [BSP 声明](../firmware/components/bsp/idf_component.yml)，解析结果见 [dependencies.lock](../firmware/dependencies.lock)。

## 构建与完整镜像

构建使用 `firmware/resources/` 中的已整理资源。首次下载缺失的 ESP-IDF 组件可能需要联网。旧配置曾关闭音频分区时，可用 `idf.py -C firmware -DIFRUIT_PACK_AUDIO=ON build` 恢复默认打包。

若要生成根目录常用的单文件烧录包：

```bat
idf.py -C firmware build
idf.py -C firmware merge-bin
copy /y firmware\build\merged-binary.bin firmware\build\ai-passport-ifruit-full.bin
```

完整包包含引导、分区表、程序和音频，烧录起始地址为 `0x0`，会覆盖 8MB Flash 中的 NVS。保留设置和进度的日常更新使用 `idf.py -C firmware -p COM5 flash`，串口按实际替换。

首次安装也可用完整包直接烧录：

```bat
python -m esptool --chip esp32c3 --port COM5 --baud 460800 write_flash 0x0 firmware\build\ai-passport-ifruit-full.bin
```

首次安装也可用完整包直接烧录：

```bat
python -m esptool --chip esp32c3 --port COM5 --baud 460800 write_flash 0x0 build\ai-passport-ifruit-full.bin
```

## 更新设备资源

修改预制内容、字库或图片后，先安装可选的资源转换依赖，再生成和整理设备输入：

```bat
python -m pip install -r tools/requirements.txt
python -B -m tools.pack.prepare_content
python -B -m tools.pack.prepare_ui
python -B -m tools.pack.prepare_firmware
idf.py -C firmware build
```

更新对白或音频映射时，先归档现有 `.local/audio/`，在整理设备输入之前运行 `python -B -m tools.pack.prepare_audio`。脚本需要空输出目录，使用 FFmpeg 或 imageio-ffmpeg。若删除了一项设备音频，应先从 `firmware/resources/audio/` 移走对应旧文件，再整理设备输入。脚本会核对哈希和完整文件集合。

`tools/device/build.ps1` 是可选的 Windows 开发入口，依次生成内容、界面并整理设备输入，再调用 ESP-IDF。参数和其他开发工具见[工具索引](../tools/README.md)。

## 主机验证和实机截图

```bat
python -B -m tools.check.test
python -m unittest discover -s tests -p test_capture_screen.py
python -B -m tools.check.preview
```

主机检查使用 GCC。LVGL 预览使用已获取的 ESP-IDF LVGL 组件，并要求 CMake、Ninja 和 GCC 在 PATH 中。图片输出到 `.local/preview/`。版本验收结果见[验证状态](verification.md)。

根目录 `screenshot.cmd` 调用 `tools/device/capture_screen.ps1`。脚本可复用 ESP-IDF 的 Python 和 pyserial，连接设备后读取 USB Serial/JTAG 上的截图协议，逐行校验并保存 PNG。运行前请释放串口。
