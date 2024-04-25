#!/usr/bin/env python3
from dataclasses import dataclass
from functools import wraps
import logging
from math import inf
from time import sleep
from typing import Optional, Type
import lidar
import arduino
from gen import *

log = logging.getLogger("bang")

class _Arduino(arduino.Channel):
    def __init__(self, uri):
        self._lookup = {}
        self._handlers = {}
        for m in AllMsgs:
            self._lookup[m.Type] = m
        super().__init__(uri)

    def _onmessage(self, type: int, body: bytes):
        t = self._lookup.get(type)
        if t is None: 
            print(f"Could not find msg for {type =}")
            return
        h = self._handlers.get(type)
        if h is None: return
        try:
            h(t.from_buffer(body))
        except Exception as e:
            print(f"While receiving msg {type=} => {e}")
    
    def send(self, msg):
        super().send(msg.Type, msg.into_buffer())

    def onmessage(self, msg):
        def fabric(func):
            if not msg.Type in self._lookup:
                raise RuntimeError(f"Unsupported msg type: {msg.Type}")
            self._handlers[msg.Type] = func
            return func
        return fabric

class _Lidar(lidar.RP):
    def __init__(self, uri):
        super().__init__(uri)

    def _onscan(self, data: tuple):
        self.closest = inf
        for dist, intencity, theta in data:
            if self.closest > dist:
                self.closest = dist
    
    def onscan(self):
        def reg(func):
            self.scan = func
            return func
        return reg

@dataclass
class Bang:
    
    uno_uri: Optional[str] = "serial:/dev/ttyUSB0"
    lidar_uri: Optional[str] = "serial:/dev/ttyACM0"

    def __post_init__(self) -> None:
        if self.uno_uri:
            self._arduino = _Arduino(self.uno_uri)
        if self.lidar_uri:
            self._lidar = _Lidar(self.lidar_uri)
        self.arduino.send(MsgTest(True))
        self.arduino.send(MsgTest(False))
        self.arduino.send(MsgTest(True))
    
    @property
    def lidar(self):
        return self._lidar

    @property
    def arduino(self):
        return self._arduino


    
