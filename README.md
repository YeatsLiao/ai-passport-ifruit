# AiPassport iFruit

在固定 FoloToy AiPassport 上还原 GTA V 麦克的 iFruit。游戏安装目录的资源和脚本是依据；第三方网站仅作旁参考。需要游戏世界配合的功能将由本地剧情事件模拟，保留原版入口与操作顺序。

已从 Legacy 1.0.3889.0 导出原始界面、纹理、音频和相关脚本，详情见 [提取发现](docs/game-findings.md)。当前桌面、联系人、来电和通话框架已使用原版图片；原版按键音已绑定。设置支持 15 张原版背景、7 套原版配色与睡眠/正常模式；Lester、Amanda、Dave 已接入原版答录机录音和播放结束返回。其余应用、字体与布局细节仍待逐页核对，不是已经完成的 1:1 复刻。未烧录、未真机试听。

- [执行计划](docs/plan.md)
- [架构](docs/architecture.md)
- [素材规范](assets/README.md)
- [验证结果](docs/verification.md)
- [路径与版本](docs/sources.json)
- [工具用法](docs/tooling.md)

## 验证与构建

主机测试：`python tools/test.py`，需要 GCC 所在目录在 PATH 中。

实际 LVGL 界面预览：`python tools/preview.py`，需要 GCC、CMake、Ninja 和 Pillow。输出 build/preview/*.png，是固件界面的主机渲染，不是真机照片。

素材校验：`python tools/assets.py check`。音频设备转换：`python tools/prepare_audio.py`，输出 build/audio；该目录须为空，旧产物需先归档。采样转换使用 FFmpeg 或已安装的 imageio-ffmpeg。

本机构建：`powershell -File tools/build.ps1 -PackAudio`。使用 ESP-IDF 5.5.5，自动生成图片数据；-PackAudio 将已准备的音频打包进素材分区镜像。没有指定 -Port 时仅构建，不烧录。

## 现阶段操作

上/下选择，短按 OK 确认，长按 OK 返回。联系人可进入通话框架，双击 OK 可触发 Trevor 来电测试。双击入口是开发演示操作。联系人中选择 Lester、Amanda 或 Dave，经过本地固定拨号等待后播放其原版答录机录音；可中途挂断，播完自动返回。其余联系人当前模拟 BUSY。来电演示尚无铃声/对白绑定，接听仅验证通话画面。设置中的铃声选项保留原版文字，尚无合成器播放；振动只模拟状态。设置已接入 NVS 保存，确认后写入，重启恢复；实体断电验收待做。

素材统一平铺在 assets 下一层分类；生成图留在 concepts，不参与界面构建。BSP 的 MIT 许可见 components/bsp/LICENSE；游戏素材保留其各自权利状态。

本地任务事件第一阶段已接入，详见 [原版依据与边界](docs/world-events.md)。启动选择角色；桌面长按上键切换角色。桌面等待可收到本地任务通知，Check List 中查看并提交模拟完成。角色外观暂共用 iFruit；随机语音来电、网页资金与任务报酬仍待核对接入。
