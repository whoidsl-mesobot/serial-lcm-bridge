serial-lcm-bridge(1) -- provides an LCM interface to serial port
============

This daemon provides an interface for a serial port over UDP by 
[Lightweight Communications and Marshalling (LCM)](LCM).

SYNOPSIS
--------

`serial-lcm-bridge` -vv -b <*baudrate*> -t <*terminator*> <*device*>

DESCRIPTION
-----------

`serial-lcm-bridge` starts a daemon to communicate with a serial device
attached to *dev*, providing output an accepting input via LCM.

OPTIONS
-------

\-?, --help
:   Give help list

\--usage
:   Give a short usage message

\-b, --baudrate
:   speed to use when communicating with the serial device

\-p, --preserve-termios
:   leave termios options alone (if you set them by, e.g., `stty`)

\-i, --initiator=initiator
:   character that signals the start of a packet

\-t --terminator=terminator
:   character that signals the end of a packet

\-q, --quiet
:   say less

\-v, --verbose
:   say more

\-V, --version
:   Print program version


EXAMPLES
--------

To start an LCM interface connected to `/dev/ttyUSB0`:

: serial-lcm-bridge /dev/ttyUSB0

This will send an LCM message any time it receives the default terminator
(0x0A) from the serial port.

To listen for serial packets starting with SOH (0x01) and ending with
ETX (0x03), pass the optional terminator and initiator arguments:

: serial-lcm-bridge -b115200 -i 02 -t 03 /dev/ttyUSB0

Note that the present implementation will block until it receives both
characters from the serial port.

LCM INTERFACE
-------------

input: accepts messages in `raw_bytes_t` on channel *dev*i

output: published messages in `raw_bytes_t` on channel *dev*o


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

