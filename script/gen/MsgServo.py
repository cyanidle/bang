import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgServo:
    servo: int
    pos: int
    Type: ClassVar[int] = 4

    @staticmethod
    def from_buffer(buff):
        return MsgServo(*MsgServo._s.unpack(buff))
    def into_buffer(self):
        return MsgServo._s.pack(getattr(self, n) for n in MsgServo._names)

MsgServo._names = tuple(f.name for f in fields(MsgServo))
MsgServo._s = struct.Struct("<hh")
        
