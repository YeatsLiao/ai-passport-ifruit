"""Resolve native ANAUD labels to AWC stream hashes, without inferring identity by listening."""
from pathlib import Path
import json
import re
import struct
import assets as a

ROOT = Path(__file__).resolve().parents[1]

def joaat(text):
    value = 0
    for byte in text.lower().encode():
        value = (value + byte) & 0xffffffff
        value = (value + (value << 10)) & 0xffffffff
        value ^= value >> 6
    value = (value + (value << 3)) & 0xffffffff
    value ^= value >> 11
    return (value + (value << 15)) & 0xffffffff

def gxt(path):
    data = path.read_bytes()
    if data[:4] != b'2TXG':
        raise ValueError('Expected GXT2')
    count = struct.unpack_from('<I', data, 4)[0]
    result = {}
    for i in range(count):
        key, offset = struct.unpack_from('<II', data, 8+i*8)
        if offset < 16+count*8 or offset >= len(data):
            raise ValueError('GXT2 text offset out of bounds')
        result[key] = data[offset:data.index(0, offset)].decode('utf-8')
    return result

def streams(path):
    data = path.read_bytes()
    if data[:4] != b'ADAT':
        raise ValueError('Expected decrypted AWC')
    flags, count = struct.unpack_from('<HI', data, 6)
    offset = 16 + (count*2 if flags & 1 else 0)
    result = {}
    for i in range(count):
        key = struct.unpack_from('<I', data, offset+i*4)[0] & 0x1fffffff
        if key in result:
            raise ValueError('Ambiguous AWC stream hash')
        result[key] = i+1
    return result

def main():
    index = ROOT/'assets/manifest.json'
    manifest = json.loads(index.read_text(encoding='utf-8')); a.validate(manifest)
    text = gxt(ROOT/'assets/ui/gta_anaud_gxt2.gxt2')
    stream_ids = streams(ROOT/'build/reference/plain_ansph.awc')
    source = '\n'.join((ROOT/'build/reference'/name).read_text() for name in
                        ['standard_global_reg.ysc.full.c', 'appcontacts.ysc.full.c'])
    labels = set(re.findall(r'"([A-Z][A-Z0-9_]*APH[A-Z0-9_]*|ANS_[A-Za-z0-9_]+)"', source))
    labels.update(['MIC_APH1', 'MIC_APH2', 'TRE_APH1', 'FRA_APH1'])
    catalog = {}
    name_map = {}
    unresolved_names = []
    for value in text.values():
        if not value.startswith('ANSPH_'):
            continue
        filename = value.lower() + '_01'
        wave_hash = joaat(filename) & 0x1fffffff
        if wave_hash not in stream_ids:
            unresolved_names.append(value)
            continue
        name_map[value] = dict(stream=stream_ids[wave_hash], wave_hash=f'{wave_hash:08x}', wave_name=filename)
    for label in sorted(labels):
        lines = []
        for n in range(1, 31):
            key = f'{label}_{n}'
            sound = text.get(joaat(key+'a'))
            if not sound:
                continue
            line = dict(key=key, sound_key=key+'a', sound=sound, subtitle=text.get(joaat(key)))
            if sound in name_map:
                line.update(name_map[sound])
                line['asset_id'] = f'gta_answerphone_stream_{line["stream"]:03}'
            else:
                line['unresolved_audio'] = True
            lines.append(line)
        if lines:
            catalog[label] = lines
    report = dict(source_gxt='gta_anaud_gxt2', source_gxt_sha256=a.digest(ROOT/'assets/ui/gta_anaud_gxt2.gxt2'),
                  algorithm='joaat(lowercase(ANAUD audio name + _01)) & 0x1fffffff == AWC stream id',
                  sound_names=name_map, unresolved_names=sorted(set(unresolved_names)), conversations=catalog)
    (ROOT/'assets/answerphone-map.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    for item in manifest['assets']:
        if not item['id'].startswith('gta_answerphone_stream_'):
            continue
        n = int(item['id'].rsplit('_',1)[1])
        matches = [name for name, meta in name_map.items() if meta['stream']==n]
        item['identification'] = f'ANAUD sound {matches[0]}; stream id hash matched; conversation metadata in answerphone-map.json' if matches else item['identification']
    index.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Matched {len(name_map)}/{len(stream_ids)} ANSPH sound names; {len(catalog)} conversation labels')
    for label in ['AMA_APH1','LES_APH2','DAV_APH1','JIM_APH1','TRE_APH1']:
        print(label, json.dumps(catalog.get(label),ensure_ascii=False))

if __name__ == '__main__':
    main()
