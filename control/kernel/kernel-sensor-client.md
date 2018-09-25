# Kernel Sensor Client

The Kernel Sensor client is a Python Class and command-line utility
that invokes sensors running as Linux Kernel Modules via a UNIX Domain
Socket.

As a Python Class, it can be incorporated into other Python programs,
primarily the Sensor Wrapper.

As a command-line utility, the Kernel Sensor client may be invoked as
a testing tool, or as a stand-alone sensor client.

## Design Approach

The author being a hack Python programmer, the design is utilitarian
and, truthfully, sloppy.


    [mdday@localhost kernel]$ ./KernelSensor.py -h
    usage: KernelSensor.py [-h] [-s SOCKET] [-d] [-e] [-r] [--state STATE]
                           [--sensor SENSOR] [-o OUTFILE] [-i INFILE]

    usage: ./KernelSensor.py [...]

    optional arguments:
      -h, --help            show this help message and exit
      -s SOCKET, --socket SOCKET
                            path to domain socket
      -d, --discover        retrieve a JSON array of loaded probes (not a full
                            json exchange)
      -e, --echo            test the controller's echo server
      -r, --records         test the controller's echo server
      --state STATE         send a state change message (off, on, low, high, etc.)
      --sensor SENSOR       target sensor for message
      -o OUTFILE, --outfile OUTFILE
                            output data to this file
      -i INFILE, --infile INFILE  read input commands from this file


In addition to being executable from the command line, the Python
class may be initialized from within another program as follows:

    class KernelSensor:
        def __init__(self,
                     args,
                     socket_name = '/var/run/kernel_sensor',
                     target_sensor = "Kernel PS Sensor",
                     out_file = '-'):

where `args` is the dictionary created by the standard `ArgumentParser` class.
(If I had more time I would remove the reduncancies between the args


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

## echo

The echo message is a simple connectivity test.

    [mdday@localhost kernel]$ sudo ./KernelSensor.py -e
    connecting to /var/run/kernel_sensor
    attempting to open - as output file
    response expected: 4.16.14-300.fc28.x86_64

    sending "echo"
    received "4.16.14-300.fc28.x86_64"

The Kernel sensor running in the server responds with its kernel
version. This proves that the UNIX domain socket is functional.

## discovery


The discovery message solicits a JSON object containing the name and
instance UUID of each kernel sensor running on the server. (See the
document _savior/control/kernel/messages.md_ for the meaning of the
discovery request and reply fields.)

    [mdday@localhost kernel]$ sudo ./KernelSensor.py -d
    connecting to /var/run/kernel_sensor
    attempting to open - as output file
    {"Virtue-protocol-version": 0.1, "request": ["6822283c13a34149ad534f6c18ce7d02", "discovery", "*"]}
    {'Virtue-protocol-version': 0.1, 'reply':
    ['6822283c13a34149ad534f6c18ce7d02', 'discovery', [['Kernel Sysfs Sensor', '00ca9c1e-71f4-4665-bc34-fd7d7ffdb66b'],
    ['Kernel LSOF Sensor', '3180ed42-59d5-4bd8-88e6-1ddb92e8fadf'],
    ['Kernel PS Sensor', '2ae5fe83-b8f1-4f96-b6d4-44e5bdadc316']]]}


## state

The state message is a multi-purpose message that can cause a specific
sensor to undergo a state change. Supported state changes are:

* off
* on
* increase (sensing level)
* decrease (sensing level)
* low
* default
* high
* adversarial
* reset

Here is an example of sending a state change (increase) message to the
default target sensor (Kernel PS Sensor):

    [mdday@localhost kernel]$ sudo ./KernelSensor.py --state increase
    connecting to /var/run/kernel_sensor
    attempting to open - as output file
    {"Virtue-protocol-version": 0.1, "request": ["19b19880dc874fd9b634d5ded8e466f0", "increase", "Kernel PS Sensor"]}
    {'Virtue-protocol-version': 0.1, reply: [19b19880dc874fd9b634d5ded8e466f0, Adversarial]}

The Kernel PS Sensor increased its state one level from _high_ to
_adversarial_.

Each sensing level has configurable runtime parameters associated with
it. These are currently controlled by the function
`process_state_message` in the kernel module file
`controller-common.c`. These parameters are also controllable via the
linux kernel sysfs file system. See lines 62-104 of
`controller-common.c` and the `module_param` macros.

`process_state_message` is a shared message handler, meaning it is the
default for all three current sensors. Each sensor may replace it with
a specific message handler during its sensor initialization. For
example, see the kernel module functions `init_sensor` and
`init_kernel_ps_sensor`.


## records

The records message will retrieve current sensing records from the
_target sensor_. At present, sending a _records request_ will
automatically cause the sensor to run and collect new records,
clearing old records.

See the docment `savior/control/kernel/messages.md` for the format of
the `records` request and response messages.

    [mdday@localhost kernel]$ sudo ./KernelSensor.py -r
    connecting to /var/run/kernel_sensor
    attempting to open - as output file
    {"Virtue-protocol-version": 0.1, "request": ["17ab9fd684d94338840fc4c38a4f0883", "records", "Kernel PS Sensor"]}
    {'Virtue-protocol-version': 0.1, reply:
    [ '17ab9fd684d94338840fc4c38a4f0883', 'Kernel PS Sensor', 'kernel-ps', '2ae5fe83-b8f1-4f96-b6d4-44e5bdadc316', '0', 'systemd'. '1','0', '1d41195975c33b89']}

    ...

    {'Virtue-protocol-version': 0.1, reply: [ '17ab9fd684d94338840fc4c38a4f0883', 'Kernel PS Sensor', 'kernel-ps', '2ae5fe83-b8f1-4f96-b6d4-44e5bdadc316', '215', 'kcontrol read &'. '24202','0', '1d41195975c33b89']}
    {'Virtue-protocol-version': 0.1, reply: [ '17ab9fd684d94338840fc4c38a4f0883', 'Kernel PS Sensor', '2ae5fe83-b8f1-4f96-b6d4-44e5bdadc316']}
    send_records_message: closing socket


The last `records` response is always empty, signifying that the
`records` request is complete.

## target sensor

The `state` and `records` messages are always targeted to a specific
sensor instance. This is the _target sensor_, and is controlled by the
`--sensor` parameter.

The _target sensor_ may be addressed by either a _name_ (e.g., "Kernel
PS Sensor") or an _instance UUID_. The _name_ is human readable, but
the _instance UUID_ is guaranteed to be unique in case there are two
or more sensors with the same name running on the server.

The _instance UUID_ is always part of the `discovery` response message
for each active sensor.

Here is an example of sending a `state` message to a sensor using an
_instance UUID_:

    [mdday@localhost kernel]$ sudo ./KernelSensor.py --sensor '3180ed42-59d5-4bd8-88e6-1ddb92e8fadf' --state default
    connecting to /var/run/kernel_sensor
    attempting to open - as output file
    {"Virtue-protocol-version": 0.1, "request": ["b384a8f896834e1f9a7eb18ac14b6459", "default", "3180ed42-59d5-4bd8-88e6-1ddb92e8fadf"]}
    {'Virtue-protocol-version': 0.1, reply: [b384a8f896834e1f9a7eb18ac14b6459, Default]}
    send_records_message: closing socket

