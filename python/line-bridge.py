#!/usr/bin/env python

import collections
import time
import select
import sys

import crcmod
import serial

import lcm
from lcmtypes import raw_bytes_t

# TODO: extend to include multiple sio<=>lio lanes

class BufferedSerialWithHandler(serial.Serial):
    """
    """
    def __init__(self, buffer_size=255, channel=None, handler=None, **kw):
        self.buffer = collections.deque(maxlen=buffer_size)
        self.readtime = None
        self.channel = channel
        self.handler = handler
        kw.update(timeout=0)
        super().__init__(**kw)

    def fill(self):
        self.buffer.extend(self.read(min(self.inWaiting(),self.buffer.maxlen)))
        self.readtime = int(time.time() * 1e6)
        if self.inWaiting() > 0:
            print('buffer full with {0} bytes waiting'.format(self.inWaiting()))

    def subscribe(self, channel, handler):
        self.channel = channel
        self.handler = handler
        return (channel, handler)

    def handle(self):
        self.fill()
        self.handler(self.channel, self.buffer) # handler is intended to be the method that inspects the buffer for delimiters (or for headers, payloads, and checksums) and publishes the appropriate parts to LCM

    def set_delimiter(self, delimiter):
        self.delimiter = delimiter.encode(sys.stdin.encoding)


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
                time.sleep(1)
        except KeyboardInterrupt:
            print('Terminated by user.')

    def close(self):
        raise NotImplementedError

    def lcm_handler(self, channel, data):
        """Decode incoming LCM messages and write directly to serial output.
        """
        msg = raw_bytes_t.decode(data)
        # TODO: consider re-using raw_bytes_t object
        self.sio.write(''.join(c for c in msg.raw))
        # TODO: check for extra functionality using py3 string encode & decode

    def serial_delimiter_handler(self, channel, data):
        print('data: {0}'.format(data))
        print('delimiter: {0}'.format(self.sio.delimiter))
        while self.sio.delimiter in data:
            linelist = data.pop()
            while linelist[-1] is not self.sio.delimiter:
                linelist.extend(data.pop())
            msg = raw_bytes_t()
            msg.timestamp = self.sio.readtime
            msg.size = len(linelist)
            msg.raw = linelist # TODO: is this faster using a string?
            self.lio.publish(channel, msg.encode())

    def serial_line_handler(self, channel, data):
        self.sio.set_delimiter(b'\r')
        self.serial_delimiter_handler(channel, data)

    def serial_header_payload_checksum_handler(self, channel, data):
        pq = collections.deque(maxlen=self.sio.preamble_len)
        while pq.count(self.sio.preamble_char) < self.sio.preamble_len:
            pq.append(data.pop())
        header = ''.join(c for c in pq)
        while len(header) < self.sio.header_struct.size:
            header += data.pop()
        payload_size = self.sio.header_struct.unpack(header)[self.sio.header_payload_size_index]
        payload = data.pop()
        while len(payload) < payload_size:
            payload += data.pop() # TODO: There's probably a better way to do this.
        checksum = data.pop()
        while len(checksum) < self.sio.crc_struct.size:
            checksum += data.pop()
        checksum_read = self.sio.crc_struct.unpack(checksum)[0]
        checksum_calc = self.sio.crc(payload)
        if checksum_calc is checksum_read:
            hpc = header + payload + checksum
            msg = raw_bytes_t()
            msg.timestamp = self.sio.readtime
            msg.size = len(hpc)
            msg.raw = hpc
            self.lio.publish(channel, msg.encode())
        else:
            print('checksum mismatch: read {0}, calulated {1}'.format(checksum_read, checksum_calc))
            # TODO: publish on a different channel?


def main(port, channel=None, baudrate=38400, verbosity=0):
    if channel is None: channel = port.split('/')[-1]
    lane = Lane(verbosity=verbosity)
    lane.sio.port = port
    lane.sio.baudrate = baudrate
    lane.sio.subscribe('.'.join((channel, 'in')), lane.serial_line_handler)
    lane.lio.subscribe('.'.join((channel, 'out')), lane.lcm_handler)
    lane.open() # TODO: add optional verbose output

# For Rowe Technologies ADCP:
# port_name = 'ttyS0'
# lane = Lane()
# lane.sio.port = '/dev/' + port_name
# lane.sio.baudrate = 9600
# lane.sio.preamble_char = b'\x80'
# lane.sio.preamble_len = 16
# lane.sio.header_struct = struct.Struct(8*'i')
# lane.sio.header_payload_size_index = 6
# lane.sio.crc_struct = struct.Struct('<I')
# lane.sio.crc = crcmod.predefined.mkPredefinedCrcFun('xmodem')
# lane.sio.subscribe(port_name, lane.serial_header_payload_checksum_handler)
# lane.lio.subscribe(port_name, lane.lcm_handler)
# lane.open() # TODO: add optional verbose output

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
