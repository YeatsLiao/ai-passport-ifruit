# 本地任务事件 · 第一阶段

本阶段已实现角色选择、随机任务通知、Open Missions / Progress、模拟完成与独立存档。随机语音来电、任务报酬、网页账户和角色专属手机外观尚未实现。

## 原版依据

appchecklist.ysc 的 func_30 设置 Check List 首页：CELL_3301 / Open Missions、CELL_3302 / Progress；func_31 设置 Missions、Hobbies and Pastimes、Strangers and Freaks、Random Events、Miscellaneous。原始脚本已索引为 gta_appchecklist_ysc。

standard_global_reg.ysc 的 func_700/701 注册任务文字键。当前选取 M_LS1、M_FAM1、M_ARM1、M_ARM2、M_TRV1、M_TRV2 六个原版任务名称，构建时从原始 global_american.gxt2 读取显示文字，名称不手工改写。

## 本地改编边界

三角色各两个任务的白名单、任务出现时间和在详情页提交完成，是设备的本地回放玩法，并非原版任务解锁规则。当前没有将任何原版录音硬配给这些委托，也没有声称任务已经有对应语音电话。

Texts 页面显示的 Local replay available 是本地系统通知，不是某角色的原版短信。任务完成仅记录回放进度，不发未经核对的现金报酬。进度页显示本设备的计数，不是 GTA 存档的总完成度。

角色选择属于本地启动界面。三角色当前共用现有 iFruit 预览界面；Franklin/Trevor 原版手机外观仍待单独接入，不能把当前界面称为三套完整原版手机。

## 操作与调度

启动后选择角色；桌面长按上键再次选择。上/下选择、OK 确认、长按 OK 返回。切换角色不改写其他角色的任务进度。

角色在桌面空闲、且非 Sleep Mode 时累计等待；首次等待 45 秒，后续间隔 90–180 秒。只在本角色尚无待办时从未完成池中选一个任务。进入通话、菜单、角色选择或睡眠时停止倒计时，不积累补发。时间值均为本地演示设计。

任务出现时播放已确认的麦克短信提示音，Texts 中显示任务名称；Check List → Open Missions → 任务详情 → OK 完成本地回放。任务只能完成一次，不会重复记数。两个任务完成后池为空，不无限重复派发。

## 状态与存储

world.c 是无 ESP-IDF/LVGL 依赖的状态模块。world-events.json 平铺记录任务键和原版/改编边界。prepare_content.py 在构建时生成只读文字表。

world_t 保存当前角色、三个角色的 active/done 位集合、随机状态和等待时间。NVS 的 world_v1 与原设置键分开，验证版本、长度、角色边界、位掩码、active/done 不重叠和等待范围后才恢复。只在角色或任务状态改变时保存，不每帧写 Flash；断电测试仍待真机。

## 验证

主机测试覆盖暂停倒计时、角色任务范围、错误角色不能提交、重复完成拒绝、任务不重复、角色状态隔离、非法位集合拒绝，以及从任务通知进入列表、详情、完成和切换角色的实际 phone_key 流程。

同一份 UI 与状态机已在桌面 LVGL 渲染角色选择、Check List、Open Missions、任务详情和 Progress 并检查图片。实际硬件显示、提示音和重启恢复尚未验收。
