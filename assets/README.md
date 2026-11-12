# 素材目录

- `ui/`、`icons/`、`wallpapers/` 和 `fonts/` 保存界面文件、解码贴图和字体。
- `Chars/` 保存用户提供的人物头像，`portraits/` 保存游戏归档中的头像原件和解码文件。
- `sounds/`、`voices/` 保存音效和原声。
- `concepts/` 收录早期概念图。`screenshots/` 收录 README 使用的主机 LVGL 预览。实机截图保存到项目根目录下的 `.local/screenshots/`。

## 来源与设备转换

[素材清单](catalog/manifest.json)记录游戏素材的路径、来源、归档成员、SHA-256、解码状态和权利信息。WAV 条目另含采样率、声道、位深和时长。更多来源见[游戏资源发现](../docs/game-findings.md)和[项目来源记录](../docs/sources.json)。

`catalog/` 保存联系人、剧情对白、邮件短信、界面裁切、设置和播放映射。[来电标题](catalog/story-call-titles.json)根据对白整理。[出租车电话](catalog/taxi-calls.json)记录三位主角的原声叫车序列和出处。

设备输入统一放在 [firmware/resources](../firmware/resources)。对白等长音频使用 GSM WAV，短音效使用 PCM WAV。设备音频索引记录转换文件及其原件对应关系。重新生成的命令见[开发与素材更新](../docs/development.md)。

游戏原件与解码结果保留原有权利状态。BSP 的许可单独见 [LICENSE](../firmware/components/bsp/LICENSE)。
