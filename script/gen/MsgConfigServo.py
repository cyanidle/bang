import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgConfigServo:
    num: int
    channel: int
    speed: int
    minVal: int
    maxVal: int
    startPercents: int
    Type: ClassVar[int] = 6

    @staticmethod
    def from_buffer(buff):
        return MsgConfigServo(*MsgConfigServo._s.unpack(buff))
    def into_buffer(self):
        return MsgConfigServo._s.pack(*tuple(getattr(self, n) for n in MsgConfigServo._names))

MsgConfigServo._names = tuple(f.name for f in fields(MsgConfigServo))
MsgConfigServo._s = struct.Struct("<hhhhhB")
        
