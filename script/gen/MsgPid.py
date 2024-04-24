import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgPid:
    motor: int
    p: int
    i: int
    d: int
    Type: ClassVar[int] = 3

    @staticmethod
    def from_buffer(buff):
        return MsgPid(*MsgPid._s.unpack(buff))
    def into_buffer(self):
        return MsgPid._s.pack(getattr(self, n) for n in MsgPid._names)

MsgPid._names = tuple(f.name for f in fields(MsgPid))
MsgPid._s = struct.Struct("<biii")
        
