# 源码架构

## 目录

| 位置 | 职责 |
| --- | --- |
| [固件构建配置](../firmware/CMakeLists.txt) | ESP-IDF 工程入口，默认将设备音频打包进 `assets.bin` |
| [firmware/main](../firmware/main) | 应用源码 |
| [firmware/components](../firmware/components) | AI Passport BSP 和 GSM 解码器 |
| [firmware/resources](../firmware/resources) | 设备直接使用的图片、字体、内容、WAV 与哈希清单 |
| [assets](../assets/README.md) | 原始素材、解码原件、来源和预制 JSON |
| [tools](../tools/README.md)、[tests](../tests) | 资源加工、调试和主机检查 |
| `firmware/build/`、`firmware/managed_components/` | 固件构建结果和 ESP-IDF 组件缓存 |
| `.local/` | 本机截图、预览、提取缓存和历史产物 |

`.local/archive/` 按旧音频包、日志、主机测试、提取中间文件和旧构建归档。实机截图集中在 `.local/screenshots/`。

## 应用模块

- `main.c` 管理按键队列、主循环、音频事件和存档触发。
- `phone.c` 管理页面、按键、来电和拨号状态。`ui.c` 负责三种皮肤、LVGL 布局和阅读滚动。
- `phonebook.c`、`story.c` 读取预制的联系人、对白、邮件和短信。`communications.c` 处理记录、回执和自动来电。
- `world.c`、`simulation.c` 处理离线任务、账户和交易。
- `audio.c`、`wav.c` 挂载音频分区并流式解码。 `storage.c` 保存设置和进度，`screenshot.c` 通过 USB Serial/JTAG 导出屏幕像素。

按键回调将事件写入队列，主任务独占手机状态。音频工作任务通过单槽队列处理播放和停止请求，完成事件携带请求 ID。LVGL 更新受 BSP 锁保护，主机预览使用固定 32KB 内存池。

## 资源与分区

原始素材由工具转换到 `.local/generated/` 和 `.local/audio/`，再由 `tools/pack/prepare_firmware.py` 校验后整理到 `firmware/resources/`。`firmware/dependencies.lock` 固定已解析组件版本，`firmware/sdkconfig.defaults` 保存目标硬件的默认配置。

应用分区从 `0x10000` 开始，大小 3MiB。音频分区从 `0x310000` 开始，大小 `0x4f0000`。设置和进度使用 NVS。普通更新沿分区地址写入，完整 8MB 镜像从 `0x0` 写入。
