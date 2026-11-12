> 历史记录：其中的状态、容量和待办可能已被后续版本覆盖。当前使用说明见 [项目 README](../../README.md)，文档分类见 [索引](../README.md)。

# 通信扩容交付

当前唯一整合状态和容量见 [本批交付说明](release-2026-11-09.md)。

39通来电档案、12段主动拨号接通对话、35位默认答录机、44人联系人归档；当前角色通讯录14/11/16人。100个音频共3,283,078字节，保留0.87MiB音频安全预算。

原始来电在assets/call-scenes*.json，主动邀约在assets/call-scenes-outgoing.json，答录机在assets/phonebook.json；来源与哈希均可追溯。12段去电选自原生friends_controller的Hang Out接受分支，每组选择一套原始录音变体。

主机验证和目标编译通过，本批未烧录实机。
