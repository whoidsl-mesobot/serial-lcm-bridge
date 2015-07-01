#!/usr/bin/env python

import time
import select
import serial
import lcm
import raw_bytes_t


# TODO: Implement this as a bridge class with sio and lio attribute
# TODO: extend to include multiple sio<=>lio lanes


def publish_serial_line(raw, lio, channel):
    msg = raw_bytes_t()
    msg.timestamp = int(time.time() * 1e6)
    msg.size = len(raw)
    msg.raw = list(raw)
    lio.publish(channel, msg.encode())

def raw_handler(channel, data):
    msg = raw_bytes_t.decode(data)
    sio.write(''.join(c for c in msg.raw))

sio = serial.Serial(port='/dev/ttyS0', baudrate=9600, timeout=0)
sio.nonblocking()

lio = lcm.LCM()

ios = [sio, lio]

serial_buffer = '' # TODO: try implementing as a deque
serial_lines = []

try:
    while True:
        readable, writable, exceptional = select.select(ios, ios, ios)
        if sio in readable: 
            serial_buffer += sio.read(sio.inWaiting())
        if '/n' in serial_buffer:
            serial_lines.extend(serial_buffer.split('\n'))
            serial_buffer = serial_lines.pop(-1)
        while len(serial_lines) > 0 and lio in writable: 
            publish_serial_line(serial_lines.pop(1) + '\n', lio, channel)
        if lio in readable and sio in writable: 
            lio.handle()
except KeyboardInterrupt:
    print('Terminated by user.')
