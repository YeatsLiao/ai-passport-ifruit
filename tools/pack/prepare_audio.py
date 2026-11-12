"""Build device WAVs from indexed originals; never replace original assets."""
import argparse
import importlib.util
import json
from pathlib import Path
import subprocess
import shutil
import tempfile
import wave
import struct
ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location("assets_tool",ROOT/"tools/pack/assets.py")
a=importlib.util.module_from_spec(spec); spec.loader.exec_module(a)
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ffmpeg",default="ffmpeg")
    parser.add_argument("--speech-codec",choices=["gsm","pcm"],default="gsm")
    args=parser.parse_args()
    ffmpeg=args.ffmpeg
    if ffmpeg=="ffmpeg" and not shutil.which(ffmpeg):
        try:
            import imageio_ffmpeg
            ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
        except ImportError:
            pass
    manifest=json.loads((ROOT/"assets/catalog/manifest.json").read_text(encoding="utf-8")); a.validate(manifest)
    library={i["id"]:i for i in manifest["assets"]}
    mapping=json.loads((ROOT/"assets/catalog/playback.json").read_text(encoding="utf-8"))
    if len(mapping["contacts"])!=6: raise ValueError("Expected six contact slots")
    targets={"ringtone.wav":mapping["ringtone"],**{f"contact_{n}.wav":v for n,v in enumerate(mapping["contacts"])}}
    ring_clips=mapping.get("ring_clips",[])
    if len(ring_clips)>9: raise ValueError("At most nine original ringtone clips are supported")
    targets.update({f"ring_clip_{n}.wav":v for n,v in enumerate(ring_clips)})
    native_rings=mapping.get("native_rings",[])
    if native_rings and len(native_rings)!=3:raise ValueError("Expected three native synth rings")
    targets.update({f"native_ring_{n+1}.wav":v for n,v in enumerate(native_rings)})
    effects=mapping.get("effects",{})
    if set(effects)-{"menu_navigate","menu_accept","menu_back","text_arrive_tone","camera_shoot"}:
        raise ValueError("Unknown phone effect binding")
    targets.update({f"{name}.wav":value for name,value in effects.items()})
    if mapping.get("dial"):targets["dial.wav"]=mapping["dial"]
    for role,role_effects in mapping.get("role_effects",{}).items():
        if role not in {"1","2"} or set(role_effects)!=set(effects):
            raise ValueError("Invalid native role effect binding")
        targets.update({f"role_{role}_{name}.wav":value for name,value in role_effects.items()})
    plan=ROOT/'.local/generated/phone-audio-plan.json'
    sequences=mapping.get('sequences',{})
    if plan.exists():
        targets={k:v for k,v in targets.items() if not k.startswith('contact_')}
        sequences=json.loads(plan.read_text(encoding='utf-8'))
    output=ROOT/".local/audio"; output.mkdir(parents=True,exist_ok=True)
    # Refuse stale previous exports: caller can archive/remove files explicitly, no recursive cleanup.
    if list(output.iterdir()): raise ValueError(".local/audio must be empty; archive previous output first")
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
    for name,sequence in sequences.items():
        if not name.replace('_','').isalnum(): raise ValueError('Invalid sequence name')
        dest=output/(name+'.wav');sources=[]
        with wave.open(str(dest),'wb') as target:
            target.setparams((1,2,16000,0,'NONE','not compressed'))
            for step in sequence:
                if 'asset_id' in step:
                    item=library[step['asset_id']];sources.append(dict(id=item['id'],sha256=item['sha256']))
                    source=ROOT/'assets'/item['path']
                    with tempfile.TemporaryDirectory(dir=ROOT/'.local',prefix='sequence-') as temp:
                        converted=Path(temp)/'clip.wav'
                        subprocess.run([ffmpeg,'-nostdin','-v','error','-n','-i',str(source),'-ac','1','-ar','16000','-c:a','pcm_s16le',str(converted)],check=True)
                        with wave.open(str(converted),'rb') as clip:target.writeframes(clip.readframes(clip.getnframes()))
                elif 'pause_ms' in step:
                    if not 0<=step['pause_ms']<=10000:raise ValueError('Pause out of bounds')
                    target.writeframes(b'\0\0'*(step['pause_ms']*16))
                elif step.get('cue','').startswith('SFX_OVERLAP_'):
                    pass # Preserved in scene metadata; local sequential rendition keeps every spoken sample.
                else:raise ValueError('Unsupported audio sequence step')
        ready.append(dict(file=dest.name,source_id='sequence:'+name,sources=sources,sha256=a.digest(dest),timing='Original speech order and pauses; overlap cue retained as metadata, not reconstructed',**a.wav_info(dest)))
    if args.speech_codec=='gsm':
        for item in ready:
            if not (item['file'].startswith(('contact_','call_','ap_','event_','taxi_'))):continue
            dest=output/item['file'];encoded=dest.with_suffix('.compressed.wav')
            subprocess.run([ffmpeg,'-nostdin','-v','error','-n','-i',str(dest),'-ac','1','-ar','8000','-c:a','libgsm_ms',str(encoded)],check=True)
            encoded.replace(dest);item.update(codec='GSM_MS',sample_rate=8000,sha256=a.digest(dest))
    # Reserve filesystem overhead; this is conservative, not exact SPIFFS capacity.
    if sum((output/i["file"]).stat().st_size for i in ready)>4*1024*1024:
        raise ValueError("Audio exceeds 4MiB budget; select fewer/shorter clips")
    (ROOT/".local/audio-index.json").write_text(json.dumps(ready,indent=2)+"\n",encoding="utf-8")
    print(f"Prepared {len(ready)} WAVs; originals unchanged")
if __name__=="__main__": main()
