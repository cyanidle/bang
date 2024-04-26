import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgEcho:
    type: int
    size: int
    Type: ClassVar[int] = 9

    @staticmethod
    def from_buffer(buff):
        return MsgEcho(*MsgEcho._s.unpack(buff))
    def into_buffer(self):
        return MsgEcho._s.pack(*tuple(getattr(self, n) for n in MsgEcho._names))

MsgEcho._names = tuple(f.name for f in fields(MsgEcho))
MsgEcho._s = struct.Struct("<HI")
        
