"""Catalog already exported original phone audio; no game writes or inferred voice identities."""
import json
from pathlib import Path
import shutil
import assets as a

ROOT = Path(__file__).resolve().parents[1]
REF = ROOT / 'build/reference'

def main():
    index = ROOT / 'assets/manifest.json'
    manifest = json.loads(index.read_text(encoding='utf-8'))
    a.validate(manifest)
    entries = {item['id']: item for item in manifest['assets']}

    def add(asset_id, category, name, source_file, source_path, identification):
        source = REF / source_file
        item = dict(id=asset_id, category=category, path=f'{category}/{name}',
                    sha256=a.digest(source), source='GTA V Legacy 1.0.3889.0 / RageLib read-only export / CodeWalker AWC decoding / vgmstream r2168',
                    source_path=source_path, license='Rockstar original; no redistribution authorization recorded',
                    status='decoded' if source.suffix == '.wav' else 'extracted', identification=identification)
        if source.suffix == '.wav':
            item.update(a.wav_info(source))
        dest = a.ROOT / item['path']
        old = entries.get(asset_id)
        if old and asset_id.startswith('gta_answerphone_stream_') and 'ANAUD sound ' in old.get('identification',''):
            item['identification']=old['identification']
        if dest.exists() and (not old or old['path'] != item['path']):
            raise ValueError(f'Untracked destination: {dest}')
        # Re-export derived ringtone WAVs after fixing per-chunk AWC decryption.
        # Original AWC bytes remain untouched; all previous hashes were validated above.
        shutil.copyfile(source, dest)
        if old:
            old.clear(); old.update(item)
        else:
            manifest['assets'].append(item); entries[asset_id] = item

    for n in range(1, 10):
        add(f'gta_ringtone_stream_{n:02}', 'sounds', f'player_ringtone_{n:02}.wav', f'ringtone_{n}.wav',
            f'x64/audio/sfx/S_MISC.rpf/player_ringtones.awc#stream:{n}',
            'Original ringtone; corrected chunk decryption; in-game name and default character binding not verified')
    add('gta_frontend_awc', 'sounds', 'frontend.awc', 'frontend.awc',
        'x64/audio/sfx/RESIDENT.rpf/frontend.awc', 'Unmodified encrypted original container')
    events = {'menu_navigate': (16, 'e42fbd33'), 'menu_accept': (40, 'ad91bbc2'),
              'menu_back': (63, '31c3a2eb'), 'text_arrive_tone': (58, '3107976b')}
    for event, (stream, wave) in events.items():
        name = f'michael_{event}.wav'
        add(f'gta_michael_{event}', 'sounds', name, name,
            f'x64/audio/sfx/RESIDENT.rpf/frontend.awc#stream:{stream}; wave:{wave}',
            f'Phone_SoundSet_Michael/{event}; verified through sounds.dat54.rel; see docs/game-findings.md')
    add('gta_answerphone_awc', 'voices', 'answerphone.awc', 'ansph.awc',
        'x64/audio/sfx/SS_AC.rpf/ansph.awc', 'Original answerphone container; speaker mappings pending')
    for n in range(1, 116):
        add(f'gta_answerphone_stream_{n:03}', 'voices', f'answerphone_{n:03}.wav', f'answerphone_{n}.wav',
            f'x64/audio/sfx/SS_AC.rpf/ansph.awc#stream:{n}',
            'Original answerphone clip; speaker, subtitle, and trigger not yet verified; not bound to a contact')
    a.validate(manifest)
    index.write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(f'Catalogued {len(manifest["assets"])} assets; voice identities remain unassigned')

if __name__ == '__main__':
    main()
