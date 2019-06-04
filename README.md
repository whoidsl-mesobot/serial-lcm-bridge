This provides a bridge between LCM processes and UARTs (i.e., serial ports).

It should be useful for interfacing to many types of serial devices, providing
an easy way to specify serial communications settings, and allowing you to log
the raw serial traffic along with other serial messages.

install
-------

This project uses the autotools build system. For a clean install from the
git repo, try:

```shell
autoreconf --install; ./configure; make check; sudo make install
```

getting started
---------------

Read the usage

```shell
serial-lcm-bridge --help
```

or the manpage
```shell
man serial-lcm-bridge
```

### loopback test

* connect two serial ports with a null modem (or make two virtual ports
  using `socat`)

* in one terminal start `serial-lcm-bridge` on the first serial port:

```shell
serial-lcm-bridge -vv -t 0a -b19200 /dev/ttyUSB0
```

* in another terminal start picocom on the second serial port (at the same
  baudrate):

```shell
picocom -b19200 --omap crcrlf /dev/ttyUSB1
```

  - send a message ending with the terminating byte you specified (in the
    example above, 0x0a = \n for line feed)

  - use ctrl-a ctrl-w to send hexadecimal bytes

* in (yet) another terminal, send "TEST\r\n" in a `raw_bytes_t` LCM message
  to the device the bridge is running on:

```shell
serial-lcm-source ttyUSB0
```

  This simple test source waits ten seconds for a response, then times out.

* now, send a message from your own program using `raw_bytes_t`

alternative bridge using socat
------------------------------

For reference, `socat` provides a viable alternative if you want to bridge a
serial device across the network, and do not need the raw bytes logged to LCM.

First, on the machine without the device connected to the serial port:
```shell
socat tcp-listen:1701,reuseaddr pty,link=/tmp/ttyV0,raw,echo=0,b115200
```

Then, on the machine with the device connected to the serial port:
```shell
socat /dev/ttyUSB0,raw,echo=0,b115200 tcp:SINK_IP_ADDR:1701
```
replacing `SINK_IP_ADDR` with the ip address of the remote machine.
