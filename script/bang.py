#!/usr/bin/env python3
from dataclasses import dataclass
import logging
from math import inf
from time import sleep
from typing import Optional, Type
from .lidar import RP
from .arduino import Channel
from .gen import *

log = logging.getLogger("bang")

class _Arduino(Channel):
    def __init__(self, uri):
        self._hadmsg = False
        self._lookup = {}
        self._handlers = {}
        for m in AllMsgs:
            self._lookup[m.Type] = m
        super().__init__(uri)

    def _onmessage(self, type: int, body: bytes):
        self._hadmsg = True
        t = self._lookup.get(type)
        if t is None: 
            log.warning(f"Could not find msg for {type =}")
            return
        h = self._handlers.get(type)
        if h is None: return
        try:
            h(t.from_buffer(body))
        except Exception as e:
            log.error(f"While receiving msg {type=} => {e}")
    
    def send(self, msg):
        super().send(msg.Type, msg.into_buffer())

    def on(self, msg: Type):
        def fabric(func):
            if not msg.Type in self._lookup:
                raise RuntimeError(f"Unsupported msg type: {msg.Type}")
            self._handlers[msg.Type] = func
            return func
        return fabric

class _Lidar(RP):
    def __init__(self, uri):
        super().__init__(uri)

    def _onscan(self, data: tuple):
        pass
    
    def onscan(self):
        def reg(func):
            self._onscan = func
            return func
        return reg

@dataclass
class Bang:
    
    arduino_uri: Optional[str] = "serial:/dev/ttyUSB0"
    lidar_uri: Optional[str] = "serial:/dev/ttyACM0"

    def __post_init__(self) -> None:
        if self.arduino_uri:
            self._arduino = _Arduino(self.arduino_uri)
        if self.lidar_uri:
            self._lidar = _Lidar(self.lidar_uri)
        while not self.arduino._hadmsg: sleep(0.1)
        self.arduino.send(MsgTest(True))
        self.arduino.send(MsgTest(False))
        self.arduino.send(MsgTest(True))
    
    @property
    def lidar(self):
        return self._lidar

    @property
    def arduino(self):
        return self._arduino


    
