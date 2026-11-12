> 历史记录：其中的状态、容量和待办可能已被后续版本覆盖。当前使用说明见 [项目 README](../../README.md)，文档分类见 [索引](../README.md)。

> 历史分批记录；最新功能、数量、容量和实机验收状态以 [本批交付说明](release-2026-11-09.md) 为准。

# Audio batch: original recording ringtone previews

Implemented assets/playback.json ring_clips[0..8] referencing the indexed original gta_ringtone_stream_01..09 assets. tools/prepare_audio.py exports /assets/ring_clip_0.wav through ring_clip_8.wav using the same checked conversion and provenance index as other device audio.

Generated build/audio contains 16 WAVs totaling 1,882,692 bytes: 7 existing effects/answerphone files plus 9 recordings. Each output was checked as 16kHz, mono, PCM16. build/audio-index.json retains source IDs and hashes. Previous build/audio was moved to build/audio-before-rings-* rather than deleted. No original audio bytes were changed.

These recordings are verified original player_ringtones.awc streams, not verified names/defaults for PHONE_GENERIC_RING_01..03. The UI should label them Original clip 01..09, expose them as custom original recordings, and retain separate default/silent behavior. Original Synth default ringtone reconstruction remains pending; no generic recording has been falsely bound to it.

Investigated LS1AAUD and friends_controller for further real calls. LS1AAUD includes many on-site mission conversations: e.g. LS1A_WTF / LS1A_EXPLAIN are guarded by NPC presence and player position inside Lifeinvader, so they cannot be relabeled as phone calls. No new complete, source-verified incoming phone conversation was delivered in this batch. Existing answerphone recordings remain answerphone-only.
