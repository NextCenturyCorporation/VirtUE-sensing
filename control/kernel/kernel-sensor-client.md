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


