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
        self.handler(self.channel, self.read(self.inWaiting()))

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
    def __init__(self, sio, lio=lcm.LCM(), verbosity=0, qmax=2**13):
        self.verbosity = verbosity
        self.sio = sio
        self.lio = lio
        self.q = bytearray() # put the serial queue/buffer in the lane class instead
        self.qmax = qmax

    def open(self, port, channel=None, baudrate=9600):
        if channel is None: channel = port.split('/')[-1]
        self.sio.port = port
        self.sio.baudrate = baudrate
        self.sio.open()
        self.sio.nonblocking()
        
        self.sio.subscribe('.'.join((channel, 'in')), self.serial_handler)
        self.lio.subscribe('.'.join((channel, 'out')), self.lcm_handler)
        
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


class TextLane(Lane):

    def __init__(self, sio=SerialWithHandler(), lio=lcm.LCM(), verbosity=0, delimiter=b'\n'):
        if type(delimiter) is str: delimiter = delimiter.encode()
        self.msg = line_t()
        self.delimiter = delimiter
        super().__init__(sio, lio, verbosity)

    def lcm_handler(self, channel, data):
        """Decode incoming LCM messages and write directly to serial output.
        """
        self.sio.write(self.msg.decode(data).line)

    def serial_handler(self, channel, data):
        self.msg.timestamp = self.sio.readtime
        self.q.extend(data)
        while self.delimiter in self.q:
            h, s, self.q = self.q.partition(self.delimiter)
            self.msg.line = (h + s).decode()
            self.lio.publish(channel, self.msg.encode())
            if self.verbosity > 0: print('sent: {m.line}'.format(m=self.msg))
        if len(self.q) > self.qmax:
            print('queue too long, clearing')
            self.q.clear()


class BinaryLane(Lane):

    def __init__(self, sio=SerialWithHandler(), lio=lcm.LCM(), verbosity=0):
        self.msg = raw_bytes_t()
        super().__init__(sio, lio, verbosity)

    def lcm_handler(self, channel, data):
        """Decode incoming LCM messages and write directly to serial output.
        """
        self.sio.write(self.msg.decode(data).raw)

    def serial_handler(self, channel, data):
        self.q.extend(data)
        msg.timestamp = self.sio.readtime
        if len(self.q) > self.qmax:
            print('queue too long, clearing')
            self.q.clear()


def main(port, channel=None, baudrate=9600, verbosity=0):
    if channel is None: channel = port.split('/')[-1]
    lane = TextLane(verbosity=verbosity)
    lane.open(port, channel, baudrate)


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
    parser.add_argument('-b','--baudrate', default=9600,
        type=int, help='baud rate for serial port')
    parser.add_argument('-v','--verbosity', default=0, action='count',
        help='increase output')
    args = parser.parse_args()
    main(**args.__dict__)
