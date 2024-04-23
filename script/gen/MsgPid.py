import struct
import sys
from dataclasses import dataclass, fields

@dataclass
class MsgPid:
    motor: int
    p: int
    i: int
    d: int

    @staticmethod
    def from_buffer(buff):
        return MsgPid(*MsgPid._s.unpack(buff))
    def into_buffer(self):
        return MsgPid._s.pack(getattr(self, n) for n in MsgPid._names)

MsgPid._names = tuple(f.name for f in fields(MsgPid))
MsgPid._s = struct.Struct("<biii")
        
