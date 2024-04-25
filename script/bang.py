#!/usr/bin/env python3
from dataclasses import dataclass
from functools import wraps
import logging
from math import inf
from typing import Optional, Type
import lidar
import arduino
from gen import *

class _Arduino(arduino.Channel):
    def message(self, type: int, body: bytes):
        if hasattr(self, "handler"):
            self.handler(type, body)
        else:
            print(f"Unhandled Msg: {type=}, {body=}")
    
    def send(self, msg):
        super().send(msg.Type, msg.into_buffer())

class _Lidar(lidar.RP):
    def __init__(self, uri):
        self.closest = inf
        super().__init__(uri)

    def scan(self, data: tuple):
        self.closest = inf
        for dist, intencity, theta in data:
            if self.closest > dist:
                self.closest = dist

log = logging.getLogger("bang")

@dataclass
class Bang:
    
    uno_uri: Optional[str] = "serial:/dev/ttyUSB0"
    lidar_uri: Optional[str] = "serial:/dev/ttyACM0"

    def __post_init__(self) -> None:
        self._lookup = {}
        self._handlers = {}
        for m in AllMsgs:
            self._lookup[m.Type] = m
        if (self.uno_uri):
            self._arduino = _Arduino(self.uno_uri)
        if self.lidar_uri:
            self._lidar = _Lidar(self.lidar_uri)
        self._arduino.handler = self._arduino_handler

    def _arduino_handler(self, type, body):
        t = self._lookup.get(type)
        h = self._handlers.get(type)
        if t is None or h is None: 
            print(f"Could not find handler or msg type for {type=} (Msg = {t})")
            return
        try:
            h(t.from_buffer(body))
        except Exception as e:
            print(f"While receiving msg {type=} => {e}")

    def handler(self, msg: Type):
        def fabric(func):
            if not msg.Type in self._lookup:
                raise RuntimeError(f"Unsupported msg type: {msg.Type}")
            self._handlers[msg.Type] = func
            return func
        return fabric
    
    @property
    def lidar(self):
        return self._lidar

    @property
    def arduino(self):
        return self._arduino


    
