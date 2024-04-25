import struct
import sys
from dataclasses import dataclass, fields
from typing import ClassVar

@dataclass
class MsgConfigMotor:
    num: int
    radius: float
    angleDegrees: int
    interCoeff: float
    propCoeff: float
    diffCoeff: float
    coeff: float
    turnMaxSpeed: float
    maxSpeed: float
    ticksPerRotation: int
    Type: ClassVar[int] = 5

    @staticmethod
    def from_buffer(buff):
        return MsgConfigMotor(*MsgConfigMotor._s.unpack(buff))
    def into_buffer(self):
        return MsgConfigMotor._s.pack(*tuple(getattr(self, n) for n in MsgConfigMotor._names))

MsgConfigMotor._names = tuple(f.name for f in fields(MsgConfigMotor))
MsgConfigMotor._s = struct.Struct("<Bfiffffffi")
        
