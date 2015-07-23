#!/usr/bin/env python

import time
import select
import serial
import lcm

from lcmtypes import raw_bytes_t, line_t

# TODO: extend to include multiple sio<=>lio lanes

class SerialWithHandler(serial.Serial):
    """Lightweight wrapper around serial interface.

    Provides `handle` and `subscribe` methods to be similar to LCM interface.

    """
    def __init__(self, channel=None, handler=None, **kw):
        self.readtime = None
        self.channel = channel
        self.handler = handler
        kw.update(timeout=0)
        super().__init__(**kw)

    def handle(self):
        self.readtime = int(time.time() * 1e6)
        self.handler(self.channel, self.sio.read(self.sio.inWaiting()))

    def subscribe(self, channel, handler):
        self.channel = channel
        self.handler = handler
        return (channel, handler)

    def set_delimiter(self, delimiter):
        self.delimiter = delimiter


class BufferedSerialWithHandler(SerialWithHandler):
    """
    """
    def __init__(self, buffer_size=255, channel=None, handler=None, **kw):
        self.buffer_size = buffer_size
        self.buffer = bytearray() # use bytearray because a deque is implicitly casting bytes to ints
        super().__init__(channel, handler, **kw)

    def fill(self):
        max_to_read = min(self.inWaiting(), self.buffer_size - len(self.buffer))
        bytes_read = self.buffer.extend(self.read(max_to_read))
        if self.inWaiting() > 0:
            print('buffer full with {0} bytes waiting'.format(self.inWaiting()))

    def handle(self):
        self.readtime = int(time.time() * 1e6)
        self.fill()
        self.handler(self.channel, None) # passing the buffer as an arg wouldn't let it be changed within the handler


class Lane:
    """
    """
    def __init__(self, sio=BufferedSerialWithHandler(), lio=lcm.LCM(),
            verbosity=0):
        self.verbosity = verbosity
        self.sio = sio
        self.lio = lio

    def open(self):
        self.sio.open()
        self.sio.nonblocking()
        ios = (self.sio, self.lio)
        try:
            while True:
                readable, writable, exceptional = select.select(ios, ios, ios)
                if self.verbosity > 2: # TODO: use logging module instead
                    print('readable:{0}'.format(readable))
                    print('writable:{0}'.format(writable))
                    print('exceptional:{0}'.format(exceptional))
                if self.sio in readable and self.lio in writable:
                    self.sio.handle()
                elif self.sio in readable:
                    print('serial is readable, but LCM is not writable')
                if self.lio in readable and self.sio in writable:
                    self.lio.handle()
                elif self.lio in readable:
                    print('LCM is readable, but serial is not writable')
        except KeyboardInterrupt:
            self.close()

    def close(self):
        self.sio.close()

    def lcm_handler(self, channel, data):
        """Decode incoming LCM messages and write directly to serial output.
        """
        msg = raw_bytes_t.decode(data)
        # TODO: consider re-using raw_bytes_t object
        self.sio.write(bytearray(msg.raw))
        # TODO: check for extra functionality using py3 string encode & decode

    def serial_delimiter_handler(self, channel, data):
        msg = raw_bytes_t()
        msg.timestamp = self.sio.readtime
        while self.sio.delimiter in self.sio.buffer:
            line, self.sio.buffer = self.sio.buffer.split(self.sio.delimiter, 1)
            line += self.sio.delimiter # preserve the delimiter (for now)
            msg.size = len(line)
            msg.raw = line
            self.lio.publish(channel, msg.encode())
            if self.verbosity > 0: print('sent: {m.raw}'.format(m=msg))

    def serial_line_handler(self, channel, data):
        self.sio.set_delimiter(b'\n')
        self.serial_delimiter_handler(channel, data)


def main(port, channel=None, baudrate=38400, verbosity=0):
    if channel is None: channel = port.split('/')[-1]
    lane = Lane(verbosity=verbosity)
    lane.sio.port = port
    lane.sio.baudrate = baudrate
    lane.sio.subscribe('.'.join((channel, 'in')), lane.serial_line_handler)
    lane.lio.subscribe('.'.join((channel, 'out')), lane.lcm_handler)
    lane.open() # TODO: add optional verbose output


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description='serial-LCM bridge')
    parser.add_argument('-V', '--version', action='version',
        version='%(prog)s 0.0.1',
        help='display version information and exit')
    parser.add_argument('port', default='/dev/ttyS0',
        type=str, help='serial port to bridge')
    parser.add_argument('-c', '--channel', default=None,
        type=str, help='LCM channel to bridge')
    parser.add_argument('-b','--baudrate', default=38400,
        type=int, help='baud rate for serial port')
    parser.add_argument('-v','--verbosity', default=0, action='count',
        help='increase output')
    args = parser.parse_args()
    main(**args.__dict__)
