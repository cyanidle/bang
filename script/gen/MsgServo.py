import struct
import sys
from dataclasses import dataclass, fields

@dataclass
class Servo:
    servo: int
    pos: int

    @staticmethod
    def from_buffer(buff):
        return Servo(*Servo._s.unpack(buff))
    def into_buffer(self):
        return Servo._s.pack(getattr(self, n) for n in Servo._names)

Servo._names = tuple(f.name for f in fields(Servo))
Servo._s = struct.Struct("<hh")
        
