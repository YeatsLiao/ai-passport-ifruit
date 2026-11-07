"""Import original bytes into a shallow asset library; no game extraction here."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import wave

ROOT = Path(__file__).resolve().parents[1] / "assets"
CATEGORIES = {"icons", "wallpapers", "portraits", "sounds", "voices", "fonts", "concepts", "ui"}
def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()
def wav_info(path):
    with wave.open(str(path), "rb") as w:
        if w.getcomptype() != "NONE" or not w.getnframes():
            raise ValueError("Expected nonempty PCM WAV")
        return dict(sample_rate=w.getframerate(), channels=w.getnchannels(),
                    sample_width=w.getsampwidth(), duration_seconds=w.getnframes()/w.getframerate())
def validate(manifest):
    ids=set(); paths=set()
    for item in manifest["assets"]:
        for key in ("id","category","path","sha256","source","source_path","license","status"):
            if not item.get(key): raise ValueError(f"Missing {key}")
        if item["id"] in ids or item["path"] in paths: raise ValueError("Duplicate id/path")
        ids.add(item["id"]); paths.add(item["path"])
        rel=Path(item["path"])
        if rel.is_absolute() or len(rel.parts)!=2 or rel.parts[0] not in CATEGORIES or ".." in rel.parts:
            raise ValueError("Asset must live directly in one category")
        path=ROOT/rel
        if not path.resolve().is_relative_to(ROOT.resolve()): raise ValueError("Path escapes assets")
        if item["category"] != rel.parts[0] or digest(path)!=item["sha256"]:
            raise ValueError(f"Category or hash mismatch: {path}")
        if path.suffix.lower()==".wav":
            for key,value in wav_info(path).items():
                if item.get(key)!=value: raise ValueError(f"WAV metadata mismatch: {key}")
    tracked={i["path"] for i in manifest["assets"]}
    for category in CATEGORIES:
        for path in (ROOT/category).rglob("*"):
            if path.is_file() and path.name!=".gitkeep" and path.relative_to(ROOT).as_posix() not in tracked:
                raise ValueError(f"Unindexed asset: {path}")
def main():
    parser=argparse.ArgumentParser(description=__doc__)
    sub=parser.add_subparsers(dest="command",required=True)
    sub.add_parser("check")
    imp=sub.add_parser("import")
    imp.add_argument("file",type=Path)
    for field in ("id","category","source","source-path","license"):
        imp.add_argument("--"+field,required=True,choices=sorted(CATEGORIES) if field=="category" else None)
    args=parser.parse_args()
    index=ROOT/"manifest.json"; manifest=json.loads(index.read_text(encoding="utf-8"))
    if manifest.get("schema")!=1: raise ValueError("Unsupported manifest schema")
    validate(manifest)
    if args.command=="check":
        print(f"PASS: {len(manifest['assets'])} indexed assets"); return
    if not re.fullmatch(r"[a-z][a-z0-9_]*",args.id): raise ValueError("Use lowercase ASCII asset id")
    if any(i["id"]==args.id for i in manifest["assets"]): raise ValueError("Existing id")
    src=args.file.resolve(strict=True)
    suffix=src.suffix.lower()
    if not re.fullmatch(r"\.[a-z0-9]+",suffix): raise ValueError("Expected a simple file extension")
    if args.category in {"voices","sounds"} and suffix not in {".wav",".awc"}:
        raise ValueError("Audio originals must be AWC or PCM WAV")
    dest=ROOT/args.category/(args.id+suffix)
    if dest.exists(): raise ValueError("Refusing overwrite")
    item=dict(id=args.id,category=args.category,path=dest.relative_to(ROOT).as_posix(),sha256=digest(src),
              source=args.source,source_path=args.source_path,license=args.license,
              status="concept" if args.category=="concepts" else "extracted")
    if suffix==".wav": item.update(wav_info(src))
    shutil.copyfile(src,dest)
    manifest["assets"].append(item)
    try:
        validate(manifest)
        pending=index.with_suffix(".tmp")
        pending.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+"\n",encoding="utf-8")
        pending.replace(index)
    except Exception:
        dest.unlink(missing_ok=True)
        raise
    print("Imported",item["path"],item["sha256"])
if __name__=="__main__":
    main()
