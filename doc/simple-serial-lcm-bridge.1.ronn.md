simple-serial-lcm-bridge(1) -- provides an LCM interface to serial port
============

This daemon provides a *simple* interface for a serial port over UDP by
[Lightweight Communications and Marshalling (LCM)](LCM).

SYNOPSIS
--------

`simple-serial-lcm-bridge` <*dev*>

DESCRIPTION
-----------

`serial-lcm-bridge` starts a daemon to communicate with a serial device
attached to *dev*, providing output an accepting input via LCM.

OPTIONS
-------

\-h, --help
:   not implemented

\-b, --baudrate
:   speed to use when communicating with the serial device
:   (hard-coded to 115200)

\-V, --version
:   not implemented


EXAMPLES
--------

To start an LCM interface connected to `/dev/ttyUSB0`:

: simple-serial-lcm-bridge /dev/ttyUSB0

This will send an LCM message with up to 255 bytes of data any time it
receives data on the serial port. Since it uses `VTIME` for a blocking read
with a timeout on the serial port, the message may be delayed by up to 100
ms after the last byte arrives.

LCM INTERFACE
-------------

input: accepts messages in `raw_bytes_t` on channel *dev*<

output: publishes messages in `raw_bytes_t` on channel *dev*>


DIAGNOSTICS
-----------

This process will continue running until it receives `SIGTERM`.

ENVIRONMENT
-----------

`LCM_DEFAULT_URL`: `udpm://239.255.76.67:7667?ttl=1`

AUTHOR
------

M Jordan Stanway <m.j.stanway@alum.mit.edu>

SEE ALSO
--------

`serial-lcm-bridge(1)`, [LCM]


[LCM]: https://lcm-proj.github.io

