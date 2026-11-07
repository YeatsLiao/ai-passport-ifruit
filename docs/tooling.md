# 本地工具与复现

游戏目录只读：D:/1.Soft/Steam/steamapps/common/Grand Theft Auto V。
用户下载的 REA 源码保留在 D:/2.Project/01_hw/rea，未改为依赖安装目录。

## REA

项目 build/tooling 中安装 Node 24.22.1 和 rea-agents 6.8.0。运行入口为 `powershell -File tools/rea.ps1 doctor --json`。CLI 已用于第三方站点旁参考，不能把它理解为 GTA 专用解包器。

Codex 的 MCP 注册路径指向 build/tooling 内的已安装包；本聊天仍通过 CLI 工作。不要直接删除 build/tooling；如需清理，先把工具迁至稳定目录并更新注册。其他生成图片/音频/固件可按各自脚本重新生成。

## 资源与脚本

- GTAUtil release 2.2.13（可执行版本报告 2.2.12.0）：https://github.com/gizzdev/gtautil 。RageLib 由其程序内嵌资源解出。
- vgmstream r2168：https://github.com/vgmstream/vgmstream 。AWC 解密后解码 WAV。
- JPEXS FFDec 26.4.0：https://github.com/jindrapetrik/jpexs-decompiler 。导出 GFX 中的 ActionScript。
- 当前原生脚本解析器：https://github.com/maybegreat48/GTA-V-Script-Decompiler 。本地将下载副本的项目构建设置适配已安装 .NET 10，未改解码逻辑。旧 njames93 解析器对本版本返回 ReturnExpected，不继续采用。
- AWC/REL 格式依据：https://github.com/dexyfex/CodeWalker 。本工程是格式受限的只读辅助脚本，不宣称支持全部 GTA 音频结构。

工具包哈希见 toolchain.json。下载副本与中间归档留在忽略的 build/tooling、build/reference。

tools/gta_extract.ps1 读取归档并导出一个成员，不覆盖目标文件；重命名的嵌套归档须以 -LogicalName 传原文件名。tools/gta_decode.py 按资源图形区指针解码纹理，避开旧 RageLib 返回空纹理数据的问题。tools/gta_unpack_ysc.ps1 解包脚本供本地解析器读取。

tools/gta_awc.ps1 区分整文件和单通道音频块加密，不将解密材料写入素材库；不支持的多通道加密明确拒绝。tools/gta_soundmap.py 解析已导出的 DAT54 音效映射；未实现节点保留未解析标记，不猜对应声音。

tools/catalog_phone_audio.py 将已解码的原件入库，保留 AWC 和逐流出处；用于本次修正并重新索引之前解码错误的铃声 WAV。它不是重新从游戏自动提取全部素材的一键入口。

## 答录机对应与内容生成

tools/resolve_answerphone.py 读取原始 ANAUD、已解密 AWC 和两个原生脚本的反编译结果，输出 assets/answerphone-map.json；逐条保留对话键、字幕、声音名、波形哈希和容器流序号。未匹配名字与非录音的暂停/合成音标签保留明确标记。

tools/prepare_content.py 从原始 GXT2、phone-settings.json、phone-calls.json 生成 Flash 内容表；校验联系人绑定与 playback.json、ANAUD 关系一致。构建、主机测试、LVGL 预览都会先执行此步骤，不依赖第三方网站素材。
