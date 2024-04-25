import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgTest:
    led: int
    Type: ClassVar[int] = 7

    @staticmethod
    def from_buffer(buff):
        return MsgTest(*MsgTest._s.unpack(buff))
    def into_buffer(self):
        return MsgTest._s.pack(getattr(self, n) for n in MsgTest._names)

MsgTest._names = tuple(f.name for f in fields(MsgTest))
MsgTest._s = struct.Struct("<B")
        
