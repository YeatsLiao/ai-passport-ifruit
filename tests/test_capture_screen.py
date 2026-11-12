"""Protocol and PNG checks without a board or pyserial."""
import importlib.util,itertools,struct,unittest,zlib
from pathlib import Path
from unittest.mock import patch
spec=importlib.util.spec_from_file_location('capture',Path(__file__).resolve().parents[1]/'tools/device/capture_screen.py')
capture=importlib.util.module_from_spec(spec);spec.loader.exec_module(capture)
class Port:
 def __init__(self,data):self.data=bytearray(data)
 @property
 def in_waiting(self):return len(self.data)
 def read(self,n):out=bytes(self.data[:n]);del self.data[:n];return out
class CaptureTests(unittest.TestCase):
 def packet(self,missing=None,bad_crc=False):
  row=struct.pack('<4H',0xf800,0x07e0,0x001f,0xffff)*60;crc=0
  records=[b'boot log ignored\n@IFRUIT_BEGIN 1 240 320 RGB565LE\n']
  for y in range(320):
   crc=zlib.crc32(row,crc)
   if y!=missing:records.append(f'@IFRUIT_ROW 0 {y} 240 {row.hex()}\n'.encode())
  records.append(f'@IFRUIT_END 76800 {crc^(1 if bad_crc else 0):08x}\n'.encode())
  return b''.join(records),row
 def receive(self,data,timeout=100):
  with patch.object(capture.time,'monotonic',side_effect=itertools.count(0,.001)):
   return capture.receive(Port(data),timeout)
 def test_full_frame_and_png(self):
  packet,row=self.packet();frame,w,h=self.receive(packet);self.assertEqual((w,h),(240,320));self.assertEqual(frame,row*320)
  png=capture.png_bytes(frame,w,h);self.assertEqual(png[:8],b'\x89PNG\r\n\x1a\n');pos=8;idat=b''
  while pos<len(png):
   n=struct.unpack_from('>I',png,pos)[0];kind=png[pos+4:pos+8];data=png[pos+8:pos+8+n];crc=struct.unpack_from('>I',png,pos+8+n)[0]
   self.assertEqual(crc,zlib.crc32(kind+data))
   if kind==b'IDAT':idat+=data
   pos+=n+12
  raw=zlib.decompress(idat);self.assertEqual(len(raw),320*(1+240*3));self.assertEqual(raw[:13],bytes([0,255,0,0,0,255,0,0,0,255,255,255,255]))
 def test_crc_and_coverage(self):
  for data in [self.packet(bad_crc=True)[0],self.packet(missing=77)[0]]:
   with self.assertRaises(ValueError):self.receive(data)
 def test_duplicates_and_invalid_bounds(self):
  header=b'@IFRUIT_BEGIN 1 240 320 RGB565LE\n'
  for record in [b'@IFRUIT_ROW 0 0 1 0000\n'*2,b'@IFRUIT_ROW 240 0 1 0000\n',b'@IFRUIT_ROW 0 0 1 00\n']:
   with self.assertRaises(ValueError):self.receive(header+record)
 def test_incomplete_timeout(self):
  with self.assertRaises(TimeoutError):self.receive(b'@IFRUIT_BEGIN 1 240 320 RGB565LE\n',.01)
 def test_ready(self):
  capture.wait_ready(Port(b'log\n@IFRUIT_READY 2 FAP_SCREENSHOT_V1 screenshot\n'))
  with patch.object(capture.time,'monotonic',side_effect=itertools.count()):
   with self.assertRaises(TimeoutError):capture.wait_ready(Port(b'@SLATE_READY 2 FAP_SCREENSHOT_V1 screenshot\n'))
if __name__=='__main__':unittest.main()
