import struct
import sys
from dataclasses import dataclass, fields

@dataclass
class Odom:
    num: int
    aux: int
    ddist_mm: int

    @staticmethod
    def from_buffer(buff):
        return Odom(*Odom._s.unpack(buff))
    def into_buffer(self):
        return Odom._s.pack(getattr(self, n) for n in Odom._names)

Odom._names = tuple(f.name for f in fields(Odom))
Odom._s = struct.Struct("<bbH")
        
