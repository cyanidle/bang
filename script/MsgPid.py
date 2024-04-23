import struct
import sys
from dataclasses import dataclass, fields

@dataclass
class Pid:
    motor: int
    p: int
    i: int
    d: int

    @staticmethod
    def from_buffer(buff):
        return Pid(*Pid._s.unpack(buff))
    def into_buffer(self):
        return Pid._s.pack(getattr(self, n) for n in Pid._names)

Pid._names = tuple(f.name for f in fields(Pid))
Pid._s = struct.Struct("<biii")
        
