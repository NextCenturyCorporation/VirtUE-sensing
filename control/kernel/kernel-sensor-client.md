# Kernel Sensor Client

The Kernel Sensor client is a Python Class and command-line utility
that invokes sensors running as Linux Kernel Modules via a UNIX Domain
Socket.

As a Python Class, it can be incorporated into other Python programs,
primarily the Sensor Wrapper.

As a command-line utility, the Kernel Sensor client may be invoked as
a testing tool, or as a stand-alone sensor client.

## Design Approach

The author being a novice Python programmer, the design is utilitarian
and, truthfully, sloppy.

### Instance Variables
The `KernelSensor` class has three instance variables
that define communications with the server:

* `self.socket_name` and `self.sock` are the path and socket by which
the client communicates with the running Kernel Sensor. Currently
these instance variables must describe an `AF_UNIX` domain socket, although
they can be extended to other domain types.

The default value for `socket_name` is `/var/run/kernel_sensor`. It
may be set using the `set_socket(self, socket_name)` which will close
an existing socket and open a socket using the new name.

* `self.target_sensor` determines the sensor that will be on the
other end of the socket. `target_sensor` may be a human-readable
sensor name, or a `uuid` string. (See the `discovery` message for more
info.)

The default value for `target_sensor` is `"Kernel PS Sensor"` although
this is arbitray.

### Message Methods
Once the `socket` and `target_sensor` variables are set, you may use
an instance method to operate on a running Kernel Sensor instance.
