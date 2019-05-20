This provides a bridge between LCM processes and UARTs (i.e., serial ports).

It should be useful for interfacing to many types of serial devices, providing
an easy way to specify serial communications settings, and allowing you to log
the raw serial traffic along with other serial messages.

alternative bridge using `socat`
--------------------------------

For reference, `socat` provides a viable alternative if you want to bridge a
serial device across the network, and do not need the raw bytes logged to LCM.

First, on the machine without the device connected to the serial port:
```shell
socat tcp-listen:1701,reuseaddr pty,link=/tmp/ttyV0,rawer,b115200
```

Then, on the machine with the device connected to the serial port:
```shell
socat /dev/ttyUSB0,rawer,b115200 tcp:SINK_IP_ADDR:1701
```
replacing `SINK_IP_ADDR` with the ip address of the remote machine.
