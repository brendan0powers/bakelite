import crcmod
from typing import Optional

g_crc_func = crcmod.predefined.mkPredefinedCrcFun('crc-8')

class FrameError(RuntimeError):
  pass

class EncodeError(FrameError):
  pass

class DecodeError(FrameError):
  pass

class CRCCheckFailure(FrameError):
  pass

def _append_block(block: bytearray, output: bytearray):
  output.append(len(block)+1)
  output.extend(block)
  block.clear()

def encode(data: bytes) -> bytearray:
  block = bytearray()
  output = bytearray()
  full_block = False

  if not data:
    return b''

  for byte in data:
    full_block = False
    if byte == 0:
      _append_block(block, output)
    else:
      block.append(byte)
      if(len(block) == 254):
        _append_block(block, output)
        full_block = True
  
  if not full_block:
    _append_block(block, output)

  return output

def decode(data: bytes):
  output = bytearray()

  if not data:
    return b''

  while len(data) > 0:
    block_size = data[0]

    if block_size == 0:
      raise DecodeError("Unexpected null byte")

    if block_size > len(data):
      raise DecodeError("Block length exceeds size of available data")

    block = data[1:block_size]
    data = data[block_size:]

    output.extend(block)
    if(block_size != 255):
      output.append(0)

  if output[-1] == 0:
    output.pop()

  return output

def append_crc(data: bytes, crc=True, crc_fn=g_crc_func, crc_num_bytes=1):
  return data + crc_fn(data).to_bytes(crc_num_bytes, byteorder='little')

def check_crc(data: bytes, crc=True, crc_fn=g_crc_func, crc_num_bytes=1):
  if not data:
    raise CRCCheckFailure()

  crc_val = int.from_bytes(data[-crc_num_bytes:], byteorder='little')
  output = data[:-crc_num_bytes]

  if crc_fn(output) != crc_val:
    raise CRCCheckFailure()

  return output

class Framer():
  def __init__(self,
               encode_fn=encode,
               decode_fn=decode,
               crc=True,
               crc_fn=g_crc_func,
               crc_num_bytes=1):
    self._encode_fn = encode_fn
    self._decode_fn = decode_fn
    self._crc = crc
    self._crc_fn = crc_fn
    self._crc_num_bytes = crc_num_bytes
    self._buffer = bytearray()
    self._frame = bytearray()
  
  def encode_frame(self, data: bytes):
    if not data:
      raise EncodeError('data must not be empty')

    if self._crc:
      data = append_crc(data,
                        crc_fn=self._crc_fn,
                        crc_num_bytes=self._crc_num_bytes)
    
    return b'\x00' + encode(data) + b'\x00'
  
  def decode_frame(self) -> Optional[bytearray]:
    while self._buffer:
      byte = self._buffer.pop(0)
      
      if byte == 0:
        if self._frame:
          try:

            decoded = self._decode_frame_int(self._frame)
            return decoded
          finally:
            self._frame.clear()
      else:
        self._frame.append(byte)
    
    return None


  def clear_buffer(self):
    self._buffer.clear()
    self._frame.clear()

  def append_buffer(self, data: bytes):
    self._buffer.extend(data)
  
  def _decode_frame_int(self, data):
    data = decode(data)

    if self._crc:
      data = check_crc(data,
                       crc_fn=self._crc_fn,
                       crc_num_bytes=self._crc_num_bytes)
    
    return data
