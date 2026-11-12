"""Render local DAT10 programs with a build-only adaptation of CodeWalker's VM.

Requires the original optamp.dat10.rel and official RelFile.cs/Synthesizer.cs
in .local/reference and .local/tooling. No game executable is launched or changed.
The downloaded implementation stays in ignored .local/, not in device firmware.
"""
from pathlib import Path
import json, struct, subprocess, array, hashlib, shutil, wave

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'.local/phone-synth'

def block(source, signature):
    start=source.index(signature); brace=source.index('{',start); depth=1; end=brace+1
    while depth:
        if source[end]=='{': depth+=1
        elif source[end]=='}': depth-=1
        end+=1
    return source[start:end]

def programs():
    raw=(ROOT/'.local/reference/optamp.dat10.rel').read_bytes()
    typ,size=struct.unpack_from('<II',raw); assert typ==10
    data=raw[8:8+size]; pos=8+size
    _,names=struct.unpack_from('<II',raw,pos);pos+=8+4*names
    for _ in range(names):pos=raw.index(0,pos)+1
    count=struct.unpack_from('<I',raw,pos)[0];pos+=4
    return {key:data[off:off+length] for key,off,length in
            (struct.unpack_from('<III',raw,pos+i*12) for i in range(count))}

def main():
    OUT.mkdir(exist_ok=True,parents=True)
    rel=(ROOT/'.local/tooling/RelFile.cs').read_text(encoding='utf-8-sig')
    vm=(ROOT/'.local/tooling/Synthesizer.cs').read_text(encoding='utf-8-sig')
    methods=['public static Instruction[] DisassembleGetInstructions',
             'public static int SizeOf','public static Instruction Decode',
             'private static void HandleOpcode','public enum ParameterType',
             'public enum Opcode : byte']
    code='using System;using System.IO;using System.Linq;using System.Collections.Generic;\nnamespace CodeWalker.GameFiles {\n'
    code+='public class Dat10SynthVariable {public uint Name;public float Value;}\npublic class Dat10Synth {\n'
    code+='public int RegistersCount,BuffersCount,StateBlocksCount;public byte[] Bytecode,OutputsIndices;public float[] Constants;public Dat10SynthVariable[] Variables;\n'
    code+='public struct Parameter {public ParameterType Type;public ushort Value;public Parameter(ParameterType t,ushort v){Type=t;Value=v;}}\n'
    code+='public struct Instruction {public byte InputsOutputs;public Opcode Opcode;public Parameter[] Parameters;public int NumberOfInputs=>InputsOutputs>>4;public int NumberOfOutputs=>InputsOutputs&15;public int Size=>SizeOf(Opcode,NumberOfInputs,NumberOfOutputs);}\n'
    code+='\n'.join(block(rel,m) for m in methods)+'\n}}\n'
    (OUT/'Dat10.cs').write_text(code,encoding='utf-8')
    body=vm[vm.index('        private void SynthesizeFrame()'):]
    body=body.replace('FrameSynthesized?.Invoke(this, EventArgs.Empty);','')
    # Preserve the official execution and DSP code; replace only playback plumbing.
    prefix='''using System;using CodeWalker.GameFiles;
namespace CodeWalker.Utils {static class RandomRange {public static float NextFloat(this Random r,float lo,float hi)=>(float)(lo+r.NextDouble()*(hi-lo));} public class Synthesizer {
public const int BufferSize=256,SampleRate=48000;
public Dat10Synth Synth;public float[] Registers;public float[][] Buffers;public StateBlock[] StateBlocks;
private Dat10Synth.Instruction[] instructions;private bool stop;private Random rnd=new Random(1);
public void Init(Dat10Synth s){Synth=s;instructions=Dat10Synth.DisassembleGetInstructions(s.Bytecode,s.Constants,s.Variables);Registers=new float[s.RegistersCount];Buffers=new float[s.BuffersCount][];for(int i=0;i<Buffers.Length;i++)Buffers[i]=new float[256];StateBlocks=new StateBlock[s.StateBlocksCount];}
public bool Stopped=>stop;
public float[] Render(){SynthesizeFrame();return Buffers[Synth.OutputsIndices[0]];}
public string Listing()=>string.Join("\\n",Array.ConvertAll(instructions,i=>i.Opcode.ToString()));
'''
    (OUT/'Synthesizer.cs').write_text(prefix+body,encoding='utf-8')
    (OUT/'Render.csproj').write_text('<Project Sdk="Microsoft.NET.Sdk"><PropertyGroup><OutputType>Exe</OutputType><TargetFramework>net10.0</TargetFramework><EnableDefaultCompileItems>true</EnableDefaultCompileItems></PropertyGroup></Project>')
    (OUT/'Program.cs').write_text('''using System;using System.IO;using CodeWalker.GameFiles;using CodeWalker.Utils;
class Program {static void Main(string[] args){using var br=new BinaryReader(File.OpenRead(args[0]));if((br.ReadUInt32()&255)!=3)throw new Exception("Not synth");br.ReadUInt32();var s=new Dat10Synth();s.BuffersCount=br.ReadInt32();s.RegistersCount=br.ReadInt32();var outputs=br.ReadInt32();s.OutputsIndices=br.ReadBytes(4);var size=br.ReadInt32();s.StateBlocksCount=br.ReadInt32();br.ReadInt32();s.Bytecode=br.ReadBytes(size);int n=br.ReadInt32();s.Constants=new float[n];for(int i=0;i<n;i++)s.Constants[i]=br.ReadSingle();n=br.ReadInt32();s.Variables=new Dat10SynthVariable[n];for(int i=0;i<n;i++)s.Variables[i]=new Dat10SynthVariable{Name=br.ReadUInt32(),Value=br.ReadSingle()};if(br.BaseStream.Position!=br.BaseStream.Length)throw new Exception("Trailing bytes");var vm=new Synthesizer();vm.Init(s);File.WriteAllText(args[1]+".ops",vm.Listing());using var wr=new BinaryWriter(File.Create(args[1]));int frames=int.Parse(args[2]);for(int i=0;i<frames;i++)foreach(float x in vm.Render()){if(!float.IsFinite(x))throw new Exception("Nonfinite");wr.Write(x);}Console.WriteLine(args[0]+" rendered "+frames+" frames");}}
''',encoding='utf-8')
    subprocess.run(['dotnet','.local',str(OUT/'Render.csproj'),'-c','Release','--nologo'],check=True)
    nodes=programs()
    for key in [0xe9e42095,0xdf4f0b6b,0x54079c4c]:
        path=OUT/f'{key:08x}.dat';path.write_bytes(nodes[key])
        subprocess.run(['dotnet',str(OUT/'bin/Release/net10.0/Render.dll'),str(path),str(path.with_suffix('.f32')),str(1125 if key==0xe9e42095 else 750)],check=True)
    compose()

def compose():
    """Native graph: one-shot + 1500ms silence; Trevor two 500ms retriggers + 100ms."""
    import imageio_ffmpeg
    ffmpeg=imageio_ffmpeg.get_ffmpeg_exe()
    def floats(path):
        a=array.array('f');a.frombytes(path.read_bytes());return a
    for key,cutoff in [('e9e42095',10927),('df4f0b6b',5268)]:
        # Keep complete 256-sample synthesis frames through the last audible frame.
        a=floats(OUT/(key+'.f32'))
        end=max(i for i,v in enumerate(a) if abs(v)>1e-7)+1
        end=(end+255)//256*256
        a=a[:end]+array.array('f',[0.0])*(48000*1500//1000)
        raw=OUT/(key+'-cycle.f32');raw.write_bytes(a.tobytes())
        subprocess.run([ffmpeg,'-y','-v','error','-f','f32le','-ar','48000','-ac','1','-i',str(raw),'-af',f'lowpass=f={cutoff}', '-ar','16000','-c:a','pcm_s16le',str(OUT/f'native_ring_{1 if key=="e9e42095" else 2}.wav')],check=True)
    # Original frontend container, waveform hash 152c8678, stream 76 (not guessed).
    layer=OUT/'trevor-layer.wav'
    subprocess.run([str(ROOT/'.local/tooling/vgmstream/vgmstream-cli.exe'),'-s','76','-o',str(layer),str(ROOT/'.local/reference/plain_frontend.awc')],check=True,stdout=subprocess.DEVNULL)
    raw=OUT/'trevor-layer.f32'
    subprocess.run([ffmpeg,'-y','-v','error','-i',str(layer),'-ar','48000','-ac','1','-f','f32le',str(raw)],check=True)
    sample=floats(raw);synth=floats(OUT/'54079c4c.f32')
    cycle=array.array('f',[0.0])*52800
    for offset in [0,24000]:
        for i in range(24000):
            envelope=min(1.0,max(0.0,(24000-i)/2400))
            v=(sample[i] if i<len(sample) else 0.0)+synth[i]*10**(-12/20)
            cycle[offset+i]+=v*envelope
    assert max(map(abs,cycle))<1.0,'Clipping native mix'
    raw=OUT/'trevor-cycle.f32';raw.write_bytes(cycle.tobytes())
    subprocess.run([ffmpeg,'-y','-v','error','-f','f32le','-ar','48000','-ac','1','-i',str(raw),'-ar','16000','-c:a','pcm_s16le',str(OUT/'native_ring_3.wav')],check=True)
    index=ROOT/'assets/catalog/manifest.json';manifest=json.loads(index.read_text(encoding='utf-8'))
    for n in range(1,4):
        src=OUT/f'native_ring_{n}.wav';dest=ROOT/f'assets/sounds/native_ring_{n}.wav';shutil.copyfile(src,dest)
        with wave.open(str(src),'rb') as w: info=dict(sample_rate=w.getframerate(),channels=w.getnchannels(),sample_width=w.getsampwidth(),frames=w.getnframes(),duration_seconds=w.getnframes()/w.getframerate())
        item=dict(id=f'gta_native_ring_{n}',category='sounds',path=f'sounds/native_ring_{n}.wav',sha256=hashlib.sha256(src.read_bytes()).hexdigest(),source='Local GTA V Legacy original DAT10 bytecode / offline CodeWalker synthesis rendition',source_path=f'x64/audio/audio_rel.rpf/config/sounds.dat54.rel#PHONE_GENERIC_RING_{n:02}',license='Rockstar original program and sounds; no redistribution authorization recorded',status='rendered',identification='Original synth program, native sequence timing and Trevor recorded layer; offline DSP rendition, not a game-loopback recording',**info)
        manifest['assets']=[x for x in manifest['assets'] if x['id']!=item['id']];manifest['assets'].append(item)
    index.write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    provenance={'source_sha256':hashlib.sha256((ROOT/'.local/reference/optamp.dat10.rel').read_bytes()).hexdigest(),'renderer_source':'https://github.com/dexyfex/CodeWalker/blob/master/CodeWalker/Utils/Synthesizer.cs','renderer_sha256':hashlib.sha256((ROOT/'.local/tooling/Synthesizer.cs').read_bytes()).hexdigest(),'programs':['e9e42095','df4f0b6b','54079c4c'],'native_graph':'Phone Generic 01/02: one shot + 1500ms pre-delay; 03: two retriggers, 500ms, 450ms hold/50ms release, synth -12dB + frontend/152c8678, then 100ms pause','limitations':'CodeWalker offline DSP implementation is not the proprietary game engine; filter/envelope shapes require in-game listening comparison.'}
    (ROOT/'assets/catalog/native-ringtones.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
