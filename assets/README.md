# 素材目录

每类只用一层目录：icons、wallpapers、portraits、sounds、voices、fonts、ui、concepts。YTD/GFX/AWC 原始导出文件与其 PNG/WAV 解码结果同类平铺；设备转换产物放 build/generated 和 build/audio，不覆盖原件。

manifest.json 是唯一索引。每项记录 id、category、path、sha256、source、source_path、license、status。extracted 为原始导出，decoded 为解码结果，concept 为生成概念图；source_path 保留归档内路径和流编号。WAV 额外记录采样率、声道、位深和时长。所有哈希从真实文件计算。

本轮整理了 213 个中央手机界面/纹理文件和 268 张解码图片，以及脚本、文字库和音频。完整范围与缺口见 ../docs/game-findings.md，不能据此断言游戏里所有手机网页/字体/对白均已找齐。

## 播放映射

playback.json 只保存已确认素材 ID；null 表示未绑定。当前 effects 中有麦克导航、确认、返回、短信到达音。前三种接入按键状态机，短信音尚未有事件触发。

ringtone 保持 null，麦克默认 PHONE_GENERIC_RING_01 的合成器尚未实现。contacts 已绑定 Lester、Amanda、Dave 三段原版答录机声音，依据在 phone-calls.json 和 answerphone-map.json；其他联系人留空。answerphone_001 等文件名仅代表容器流序号，身份通过 ANAUD 的对话键、声音名与 AWC 哈希关系另行记录。不得生成仿声或臆造字幕。

phone-settings.json 保留原版设置文字键、背景顺序和配色值，prepare_content.py 从原始 GXT2 读取显示文字，prepare_ui.py 生成壁纸和图标。phone-calls.json 保存所选本地通话场景与已确认录音出处。

tools/prepare_audio.py 验证索引后，生成 16kHz/mono/PCM16 设备 WAV 和 build/audio-index.json，保守限制 4MiB。电脑素材库不等于全部都装入设备。

## 历史概念图

concepts 三张图片仅作历史讨论；concept_initial 含已弃用的工具功能。它们不是原版素材或真机照片，不参与固件渲染。

游戏原件与衍生解码结果保留原有权利状态，不继承 BSP 或工具的开源许可。密钥材料不放在 assets，也不进入设备镜像。
