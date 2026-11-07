"""Compile pure-C phone logic on the host; also test asset ingestion in isolation."""
import argparse
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import wave
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(); p.add_argument("--cc",default="gcc"); a=p.parse_args()
(ROOT / "build").mkdir(exist_ok=True)
subprocess.run([sys.executable,str(ROOT/'tools/prepare_content.py')],check=True)
with tempfile.TemporaryDirectory(prefix="ifruit-test-", dir=ROOT / "build", ignore_cleanup_errors=True) as temp:
    work=Path(temp); binary=work/"phone_test.exe"
    subprocess.run([a.cc,"-std=c11","-Wall","-Wextra","-Werror","-I",str(ROOT/"main"),str(ROOT/"main/phone.c"),str(ROOT/"main/world.c"),str(ROOT/'build/generated/phone_content.c'),str(ROOT/"tests/phone_test.c"),"-o",str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
    spec=importlib.util.spec_from_file_location("asset_tools",ROOT/"tools/assets.py")
    mod=importlib.util.module_from_spec(spec); spec.loader.exec_module(mod)
    library=work/"assets"; shutil.copytree(ROOT/"assets",library); mod.ROOT=library
    wav=work/"sample.wav"
    with wave.open(str(wav),"wb") as w:
        w.setparams((1,2,16000,0,"NONE","not compressed")); w.writeframes(b"\0\0"*160)
    # Validate actual device parser against valid and hostile/truncated file bytes.
    wav_exe=work/"wav_test.exe"
    subprocess.run([a.cc,"-std=c11","-Wall","-Wextra","-Werror","-I",str(ROOT/"main"),str(ROOT/"main/wav.c"),str(ROOT/"tests/wav_test.c"),"-o",str(wav_exe)],check=True)
    raw=wav.read_bytes()
    bad_rate=work/"bad_rate.wav"; data=bytearray(raw); data[24:28]=(8000).to_bytes(4,"little"); bad_rate.write_bytes(data)
    truncated=work/"truncated.wav"; truncated.write_bytes(raw[:-1])
    bad_chunk=work/"bad_chunk.wav"; data=bytearray(raw); data[40:44]=(0xffffffff).to_bytes(4,"little"); bad_chunk.write_bytes(data)
    subprocess.run([str(wav_exe),str(wav),str(bad_rate),str(truncated),str(bad_chunk)],check=True)
    info=mod.wav_info(wav); assert info["duration_seconds"]==0.01
    shutil.copyfile(wav,library/"sounds/test.wav")
    item=dict(id="test",category="sounds",path="sounds/test.wav",sha256=mod.digest(wav),source="test fixture",source_path="generated",license="test",status="extracted",**info)
    manifest=json.loads((library/"manifest.json").read_text()); manifest["assets"].append(item)
    mod.validate(manifest)
    for field,value in [("sha256","bad"),("path","../sample.wav"),("sample_rate",8000)]:
        old=item[field]; item[field]=value
        try: mod.validate(manifest)
        except (ValueError,FileNotFoundError): pass
        else: raise AssertionError("Accepted invalid "+field)
        item[field]=old
    print("PASS: asset hashing, path containment, PCM metadata")
    (library/"manifest.json").write_text(json.dumps(manifest),encoding="utf-8")
    (library/"playback.json").write_text(json.dumps({"ringtone":"test","contacts":[None,"test",None,None,None,None],"effects":{"menu_navigate":"test"}}),encoding="utf-8")
    prep_spec=importlib.util.spec_from_file_location("prepare",ROOT/"tools/prepare_audio.py")
    prep=importlib.util.module_from_spec(prep_spec); prep_spec.loader.exec_module(prep)
    prep.ROOT=work; prep.a.ROOT=library
    saved_argv=sys.argv; sys.argv=["prepare_audio.py"]
    try: prep.main()
    finally: sys.argv=saved_argv
    assert (work/"build/audio/ringtone.wav").read_bytes()==raw
    assert (work/"build/audio/contact_1.wav").read_bytes()==raw
    assert (library/"sounds/test.wav").read_bytes()==raw
    assert (work/"build/audio/menu_navigate.wav").read_bytes()==raw
    assert len(json.loads((work/"build/audio-index.json").read_text()))==3
    print("PASS: playback mapping exports device files without modifying originals")

subprocess.run([sys.executable,str(ROOT/"tools/assets.py"),"check"],check=True)
