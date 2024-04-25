import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgConfigPinout:
    num: int
    encoderA: int
    encoderB: int
    enable: int
    fwd: int
    back: int
    Type: ClassVar[int] = 8

    @staticmethod
    def from_buffer(buff):
        return MsgConfigPinout(*MsgConfigPinout._s.unpack(buff))
    def into_buffer(self):
        return MsgConfigPinout._s.pack(*tuple(getattr(self, n) for n in MsgConfigPinout._names))

MsgConfigPinout._names = tuple(f.name for f in fields(MsgConfigPinout))
MsgConfigPinout._s = struct.Struct("<bbbbbb")
        
