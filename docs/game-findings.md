# 原版文件发现 · 2026-11-07

依据为本机 GTA V Legacy 1.0.3889.0，Steam build 24129523。所有游戏归档以只读方式打开，未改安装目录。

## 画面和脚本

基础资源来自 `x64b.rpf/data/cdimages/scaleform_generic.rpf`，同名更新优先来自 `update/update.rpf/x64/data/cdimages/scaleform_generic.rpf`。213 个原始文件包括 cellphone_ifruit.gfx、提醒弹窗、手机图标纹理、壁纸和 char_* 联系人头像纹理；共解码 268 张 PNG。原始 YTD/GFX 和解码图均保留，具体归档路径及 SHA-256 见 assets/manifest.json。

GFX 舞台为 256×344。HOMEMENU_IFRUIT.as 给出 3×3 网格、初始选择索引 4、选中图标 120% 与 250ms 过渡；CONTACTLIST.as 给出五行联系人列表和 48 单位行高。当前 LVGL 实现做了 240×320 适配，尚未完整复现过渡、字体和全部布局。

原生脚本取自 `update/update2.rpf/x64/levels/gta5/script/script_rel.rpf`。cellphone_controller.ysc 反编译结果约第 7340 行起明确设置故事模式槽位：0 Email、1 Texts、2 Check List、3 Quick Save、4 Contacts、5 Settings、6 Snapmatic、7 Internet；Trackify 在对应剧情全局变量开启后占第 8 槽。应用 ID、图标帧 ID 与槽位不同，不混用。

global_american.gxt2 保存原版英文文字；当前解析出的手机文字键在 build/reference/original-phone-text.json。尚未导出原版中文语言包或确认完整字体方案。不能将 Montserrat 近似字体标为原版字体。

## 声音对应

麦克音效通过 `update/update.rpf/x64/audio/config/sounds.dat54.rel` 的 Phone_SoundSet_Michael 映射到 `x64/audio/sfx/RESIDENT.rpf/frontend.awc`：

- Menu_Navigate：wave e42fbd33，流 16，REL 数据偏移 910487。
- Menu_Accept：wave ad91bbc2，流 40，偏移 910463。
- Menu_Back：wave 31c3a2eb，流 63，偏移 910641。Hang_Up 包装节点指向同一声音。
- Text_Arrive_Tone：wave 3107976b，流 58，偏移 910439。

映射追踪结果在 build/reference/phone-soundmap.json。前三种音效已由按键状态事件触发；短信音尚未有到达事件。通话/来电状态忽略导航音，避免按键覆盖主音频。

`x64/audio/sfx/S_MISC.rpf/player_ringtones.awc` 解出 9 段铃声。这 9 段并未证实是角色默认铃声。standard_global_reg 的 func_689 明确麦克默认 PHONE_GENERIC_RING_01；REL 追踪显示其涉及 ModularSynthSound，不能拿导出 WAV 序号代替。当前不绑定默认来电铃声。

`x64/audio/sfx/SS_AC.rpf/ansph.awc` 解出 115 段答录机录音。全部以流编号存入 voices。standard_global_init 将 Global_24046 设为 ANAUD，据此提取原版 anaud.gxt2。对话的 LABEL_1 是字幕，LABEL_1a 是音频名称；音频名称转小写加 _01 后做 JOAAT 并取低 29 位，与 AWC 流 ID 比对，115 个流全部获得名称对应。文字库另有 5 个 ANSPH 名称在本容器中无匹配，保留 unresolved。解析出 76 组对话标签，不等于全部剧情通话已提取。

standard_global_reg / func_684 将 Lester 默认答录机设为 LES_APH2，Amanda 为 AMA_APH1，Dave 为 DAV_APH1。ANAUD 将其分别指向 ANSPH_BVAA / 流 72、ANSPH_AIAA / 流 95、ANSPH_BUAA / 流 37，原文与哈希链保存在 assets/answerphone-map.json，精选场景保存在 assets/phone-calls.json。三段原声已接入本地拨号演示；对白播放结束或失败后返回联系人。Jimmy 的三段对白与两个 pause 标签已解析，尚未接入。

答录机尾音标签 SFX_Phone_Generic_Answerphone_Beep_04 去除 SFX_ 前缀后可在 REL 中定位 EnvelopeSound（650ms hold）→ ModularSynthSound。该合成器尚未移植，不用另一段录音替代结尾提示音。

## 解码修正和验收界限

AWC 既有全文件加密，也有内部音频块加密。全文件解密后不能再次解密音频块；错误处理仍可能让解码器输出有时长的噪声 WAV。已按 CodeWalker 格式分别处理，并重新导出 9 段铃声及其哈希。原始 AWC 保持原字节；密钥材料仅留在忽略的本地 build/reference，不进入素材库和设备镜像。

当前校验包括来源映射、文件结构、PCM 参数、波形检查和哈希；这些不能替代实际试听。原件/设备版听感以及真机音频输出尚未验收。

## 提取范围

当前 634 条索引包含 3 张历史生成概念图。不是 628 个都属于游戏原始文件，也不是全游戏手机素材完整性证明。网页、其他归档字体、任务对话与字幕关系、剧情解锁名单仍需继续检索。设备镜像目前打包已确认的 4 个短音效和 3 段答录机原声，不把电脑库中所有内容强塞进 Flash。

## 设置还原依据

standard_global_reg / func_689 初始化设置，appSettings / func_34 按原生优先级排序。麦克故事模式为 Background、Profile、Ringtone、Theme、Vibrate；Invite Sound 是 Online 入口，不加入此版本。Profile 的 Quiet Mode、背景 1..3 和主题的占位项由原脚本禁用，未作为可选项加入。

15 张可用背景名称与顺序来自 CELLPHONE_IFRUIT.as/backgroundImageArray，7 套颜色来自其 style 数组。文字从 global_american.gxt2 按原键读取；应用时只在确认后改变状态，取消返回保留已应用值。睡眠模式阻止本地来电；振动只有模拟状态，铃声暂只能选择原版标签，未实现预览或实际铃声。设置已接入 NVS 保存，确认后写入，重启恢复；实体断电验收待做。
