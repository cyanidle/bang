import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgOdom:
    num: int
    aux: int
    ddist_mm: int
    Type: ClassVar[int] = 2

    @staticmethod
    def from_buffer(buff):
        return MsgOdom(*MsgOdom._s.unpack(buff))
    def into_buffer(self):
        return MsgOdom._s.pack(*tuple(getattr(self, n) for n in MsgOdom._names))

MsgOdom._names = tuple(f.name for f in fields(MsgOdom))
MsgOdom._s = struct.Struct("<bbh")
        
