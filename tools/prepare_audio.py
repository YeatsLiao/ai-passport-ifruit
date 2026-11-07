"""Build device WAVs from indexed originals; never replace original assets."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import shutil
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location("assets_tool",ROOT/"tools/assets.py")
a=importlib.util.module_from_spec(spec); spec.loader.exec_module(a)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg",default="ffmpeg")
    args=parser.parse_args()
    ffmpeg=args.ffmpeg
    if ffmpeg=="ffmpeg" and not shutil.which(ffmpeg):
        try:
            import imageio_ffmpeg
            ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
        except ImportError:
            pass
    manifest=json.loads((ROOT/"assets/manifest.json").read_text(encoding="utf-8")); a.validate(manifest)
    library={i["id"]:i for i in manifest["assets"]}
    mapping=json.loads((ROOT/"assets/playback.json").read_text(encoding="utf-8"))
    if len(mapping["contacts"])!=6: raise ValueError("Expected six contact slots")
    targets={"ringtone.wav":mapping["ringtone"],**{f"contact_{n}.wav":v for n,v in enumerate(mapping["contacts"])}}
    effects=mapping.get("effects",{})
    if set(effects)-{"menu_navigate","menu_accept","menu_back","text_arrive_tone"}:
        raise ValueError("Unknown phone effect binding")
    targets.update({f"{name}.wav":value for name,value in effects.items()})
    output=ROOT/"build/audio"; output.mkdir(parents=True,exist_ok=True)
    # Refuse stale previous exports: caller can archive/remove files explicitly, no recursive cleanup.
    if list(output.iterdir()): raise ValueError("build/audio must be empty; archive previous output first")
    ready=[]
    for name,asset_id in targets.items():
        if asset_id is None: continue
        item=library[asset_id]
        if item["category"] not in {"voices","sounds"}: raise ValueError("Audio slot points to non-audio asset")
        source=ROOT/"assets"/item["path"]; dest=output/name
        info=a.wav_info(source)
        if info["sample_rate"]==16000 and info["channels"]==1 and info["sample_width"]==2:
            shutil.copyfile(source,dest)
        else:
            subprocess.run([ffmpeg,"-nostdin","-v","error","-n","-i",str(source),"-ac","1","-ar","16000","-c:a","pcm_s16le",str(dest)],check=True)
        meta=a.wav_info(dest)
        ready.append(dict(file=name,source_id=asset_id,source_sha256=item["sha256"],sha256=a.digest(dest),**meta))
    # Reserve filesystem overhead; this is conservative, not exact SPIFFS capacity.
    if sum((output/i["file"]).stat().st_size for i in ready)>4*1024*1024:
        raise ValueError("Audio exceeds 4MiB budget; select fewer/shorter clips")
    (ROOT/"build/audio-index.json").write_text(json.dumps(ready,indent=2)+"\n",encoding="utf-8")
    print(f"Prepared {len(ready)} WAVs; originals unchanged")
if __name__=="__main__": main()
