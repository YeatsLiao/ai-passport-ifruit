"""Generate small flash-resident tables from indexed original text and curated native configuration."""
from pathlib import Path
import json
from tools.extract.resolve_answerphone import gxt, joaat
from tools.pack import assets as a
ROOT=Path(__file__).resolve().parents[2]

def main():
    manifest=json.loads((ROOT/'assets/catalog/manifest.json').read_text(encoding='utf-8')); a.validate(manifest)
    settings=json.loads((ROOT/'assets/catalog/phone-settings.json').read_text(encoding='utf-8'))
    calls=json.loads((ROOT/'assets/catalog/phone-calls.json').read_text(encoding='utf-8'))
    text=gxt(ROOT/'assets/ui/global_american.gxt2')
    def label(key):
        value=text.get(joaat(key))
        if not value: raise ValueError(f'Original text missing: {key}')
        return json.dumps(value,ensure_ascii=True)
    cats=settings['categories']
    if [c['id'] for c in cats]!=['background','profile','ringtone','theme','vibrate','volume','brightness']: raise ValueError('Unexpected settings contract')
    if len(calls['contacts'])!=6: raise ValueError('Expected six contacts')
    library={i['id']:i for i in manifest['assets']}
    playback=json.loads((ROOT/'assets/catalog/playback.json').read_text(encoding='utf-8'))
    original_voice_text=gxt(ROOT/'assets/ui/gta_anaud_gxt2.gxt2')
    sound_map=json.loads((ROOT/'assets/catalog/answerphone-map.json').read_text(encoding='utf-8'))
    voices=[]
    for contact in calls['contacts']:
        if contact and contact['lines']!=sound_map['conversations'][contact['conversation']]:
            raise ValueError('Call metadata differs from resolved native conversation')
        lines=[line for line in contact['lines'] if line.get('asset_id')] if contact else []
        if len(lines)>1: raise ValueError('Multi-part conversations require explicit sequencer')
        for line in lines:
            if library[line['asset_id']]['category']!='voices': raise ValueError('Expected original voice')
            if original_voice_text.get(joaat(line['key']))!=line['subtitle'] or original_voice_text.get(joaat(line['sound_key']))!=line['sound']:
                raise ValueError('Call text/audio relationship differs from ANAUD')
            if sound_map['sound_names'][line['sound']]['stream']!=line['stream']:
                raise ValueError('Call stream differs from verified AWC mapping')
        voices.append(bool(lines))
    expected=[next((line['asset_id'] for line in contact['lines'] if line.get('asset_id')),None) if contact else None for contact in calls['contacts']]
    if playback['contacts']!=expected: raise ValueError('Playback bindings differ from verified call metadata')
    events=json.loads((ROOT/'assets/catalog/world-events.json').read_text(encoding='utf-8'))
    rows=['#include "content.h"','#include "world.h"']
    rows.append('const char *const mission_names[WORLD_TASKS] = {'+','.join(label(m['key']) for m in events['missions'])+'};')
    rows.append('const char *const checklist_labels[7] = {'+','.join(label(k) for k in events['checklist_keys'])+'};')
    rows.append('const char *const setting_names[SET_COUNT] = {'+','.join((label(c['label']) if 'label' in c else json.dumps(c['text'])) for c in cats)+'};')
    rows.append('const unsigned setting_counts[SET_COUNT] = {'+','.join(str(len(c['options'])) for c in cats)+'};')
    for c in cats:
        if not 1<=len(c['options'])<=15: raise ValueError('Settings exceed fixed table capacity')
    rows.append('const char *const setting_options[SET_COUNT][15] = {'+','.join('{'+','.join((label(o['label']) if 'label' in o else json.dumps(o['text'])) for o in c['options'])+'}' for c in cats)+'};')
    rows.append('const uint32_t theme_colors[7][3] = {'+','.join('{'+','.join(hex((r<<16)|(g<<8)|b) for r,g,b in palette)+'}' for palette in settings['theme_colors'])+'};')
    rows.append('const bool contact_has_voice[6] = {'+','.join('true' if v else 'false' for v in voices)+'};')
    out=ROOT/'.local/generated';out.mkdir(parents=True,exist_ok=True)
    (out/'phone_content.c').write_text('\n'.join(rows)+'\n',encoding='utf-8')
    print('Prepared original settings and three verified answerphone bindings')
    from tools.pack.prepare_story import main as prepare_story
    prepare_story()
    from tools.pack.prepare_phonebook import main as prepare_phonebook
    prepare_phonebook()

if __name__=='__main__': main()
