import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgReadPin:
    pin: int
    value: int
    pullup: int
    Type: ClassVar[int] = 12

    @staticmethod
    def from_buffer(buff):
        return MsgReadPin(*MsgReadPin._s.unpack(buff))
    def into_buffer(self):
        return MsgReadPin._s.pack(*tuple(getattr(self, n) for n in MsgReadPin._names))

MsgReadPin._names = tuple(f.name for f in fields(MsgReadPin))
MsgReadPin._s = struct.Struct("<bbb")
        
