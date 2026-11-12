"""Export phone-specific frontend effects by native stream hash, read-only."""
from pathlib import Path
import json
import subprocess
from tools.pack import assets as a
from tools.extract.resolve_answerphone import streams

ROOT = Path(__file__).resolve().parents[2]
EFFECTS = ('menu_navigate', 'menu_accept', 'menu_back', 'camera_shoot', 'text_arrive_tone')
HASHES = {
    1: (0x0d116469, 0xcf4999a6, 0x103a752d, 0x4189df00, 0xd4beeadc),
    2: (0x515a0619, 0xcb37a9f9, 0x3857ec2e, 0x135a82a2, 0x8eff1288),
}

def main():
    container = ROOT/'.local/reference/plain_frontend.awc'
    lookup = streams(container)
    manifest_path = ROOT/'assets/catalog/manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    mapping_path = ROOT/'assets/catalog/playback.json'
    mapping = json.loads(mapping_path.read_text(encoding='utf-8'))
    roles = {}
    for role, hashes in HASHES.items():
        roles[str(role)] = {}
        for name, stream_hash in zip(EFFECTS, hashes):
            asset_id = f'gta_role_{role}_{name}'
            dest = ROOT/'assets/sounds'/(asset_id+'.wav')
            stream = lookup[stream_hash & 0x1fffffff]
            if not dest.exists():
                subprocess.run([str(ROOT/'.local/tooling/vgmstream/vgmstream-cli.exe'),
                    '-s', str(stream), '-o', str(dest), str(container)], check=True, stdout=subprocess.DEVNULL)
            item = dict(id=asset_id, category='sounds', path='sounds/'+dest.name,
                sha256=a.digest(dest), source='GTA V Legacy / native Phone_SoundSet / vgmstream PCM decode',
                source_path=f'x64/audio/sfx/RESIDENT.rpf/frontend.awc / wave {stream_hash:08x} / stream {stream}',
                license='Rockstar original; no redistribution authorization recorded', status='extracted',
                **a.wav_info(dest))
            manifest['assets'] = [i for i in manifest['assets'] if i['id'] != asset_id]
            manifest['assets'].append(item)
            roles[str(role)][name] = asset_id
    mapping['role_effects'] = roles
    a.validate(manifest)
    manifest_path.write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    mapping_path.write_text(json.dumps(mapping, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    added = [i for i in manifest['assets'] if i['id'].startswith('gta_role_')]
    print('Imported', len(added), 'role effects;', round(sum(i['duration_seconds'] for i in added), 3), 'seconds')

if __name__ == '__main__':
    main()
