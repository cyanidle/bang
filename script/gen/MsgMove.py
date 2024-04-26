import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgMove:
    x: int
    y: int
    theta: int
    Type: ClassVar[int] = 1

    @staticmethod
    def from_buffer(buff):
        return MsgMove(*MsgMove._s.unpack(buff))
    def into_buffer(self):
        return MsgMove._s.pack(*tuple(getattr(self, n) for n in MsgMove._names))

MsgMove._names = tuple(f.name for f in fields(MsgMove))
MsgMove._s = struct.Struct("<hhh")
        
