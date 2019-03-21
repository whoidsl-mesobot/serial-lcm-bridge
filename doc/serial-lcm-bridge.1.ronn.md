serial-lcm-bridge(1) -- provides an LCM interface to serial port
============

This daemon provides an interface for a serial port over UDP by 
[Lightweight Communications and Marshalling (LCM)](LCM).

SYNOPSIS
--------

`serial-lcm-bridge` <*dev*> <*terminator*> <*initiator*>

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

: serial-lcm-bridge /dev/ttyUSB0

This will send an LCM message any time it receives the default terminator
(0x0A) from the serial port.

To listen for serial packets starting with SOH (0x01) and ending with ETX
(0x03), pass the optional terminator and initiator arguments:

: serial-lcm-bridge /dev/ttyUSB0 $'\x03' $'\x01'

Note that the present implementation will block until it receives both
characters from the serial port.

LCM INTERFACE
-------------

input: accepts messages in `raw_bytes_t` on channel *dev*<

output: published messages in `raw_bytes_t` on channel *dev*>


DIAGNOSTICS
-----------

This process will continue running until it receives `SIGTERM`.

ENVIRONMENT
-----------

`LCM_DEFAULT_URL`: `udpm://239.255.76.67:7667?ttl=1`

TODO
----

* revise to avoid blocking serial read

AUTHOR
------

M Jordan Stanway <m.j.stanway@alum.mit.edu>

SEE ALSO
--------

`simple-serial-lcm-bridge(1)`, [LCM]


[LCM]: https://lcm-proj.github.io

