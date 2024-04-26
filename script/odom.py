from collections import namedtuple
from dataclasses import dataclass
from math import cos, radians, sin
from typing import List, Tuple
from .gen import MsgOdom, MsgConfigMotor

Position = namedtuple("Position", ("x", "y", "th"))

class _Mot:
    __slots__ = ("ddist", "conf", "rads")
    def __init__(self, conf: MsgConfigMotor) -> None:
        self.ddist = 0.
        self.conf = conf
        self.rads = radians(conf.angleDegrees)

@dataclass
class Odom:

    motors: List[MsgConfigMotor]
    theta_coeff: float = 1
    x_coeff: float = 1
    y_coeff: float = 1
    base_radius: float = 0.15
    max_delta_secs: float = 1

    def __post_init__(self):
        self._mots = tuple(_Mot(c) for c in self.motors)
        self._x = self._y = self._th = 0.
        self._hits = 0
    
    @property
    def hits(self): return self._hits

    def handle(self, msg: MsgOdom):
        if msg.num >= len(self._mots): 
            return False
        else: 
            self._hits += 1
            self._mots[msg.num].ddist += msg.ddist_mm * 1000.
            return True

    def update(self) -> Position:
        count = len(self._mots)
        for mot in self._mots:
            self._th += mot.ddist / count / self.base_radius * self.theta_coeff
        for mot in self._mots:
            self._x += mot.ddist * cos(self._th + mot.rads) / count * 2 * self.x_coeff
            self._y += mot.ddist * sin(self._th + mot.rads) / count * 2 * self.y_coeff
        return Position(self._x, self._y, self._th)

def test():
    pass

if __name__ == "__main__": test()