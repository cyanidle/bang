import struct
import sys
from dataclasses import dataclass, fields

@dataclass
class Move:
    x: int
    y: int
    theta: int

    @staticmethod
    def from_buffer(buff):
        return Move(*Move._s.unpack(buff))
    def into_buffer(self):
        return Move._s.pack(getattr(self, n) for n in Move._names)

Move._names = tuple(f.name for f in fields(Move))
Move._s = struct.Struct("<HHH")
        
