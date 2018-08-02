## running the kernel sensor
There is a shell script `/savior/control/kernel/cycle.sh` that I use for re-building and running the kernel module. Here is an excerpt:

```shell
[mdday@localhost kernel]$ pwd
/home/mdday/src/savior/control/kernel
make rebuild
sudo insmod ./controller.ko ps_repeat=1 ps_timeout=1 ps_level=2 lsof_level=2 \
     lsof_repeat=1 lsof_timeout=1 print_to_log=0
sudo lsmod | grep controller
```
## running the kernel sensing client
```shell
[mdday@localhost kernel]$  ./KernelProbe.py --help 
usage: KernelProbe.py [-h] [-s SOCKET] [-c] [-d] [-e] [-r] [-p PROBE]
                      [-f FILE] [-i INPUT]

usage: ./KernelProbe.py [--connect] [--discover] [--echo] [--socket <path>]

optional arguments:
  -h, --help            show this help message and exit
  -s SOCKET, --socket SOCKET
                        path to domain socket
  -c, --connect         issue a JSON connect message
  -d, --discover        retrieve a JSON array of loaded probes (not a full
                        json exchange)
  -e, --echo            test the controller's echo server
  -r, --records         test the controller's echo server
  -p PROBE, --probe PROBE
                        target probe for message
  -f FILE, --file FILE  output data to this file
  -i INPUT, --input INPUT
                        read input commands from this file

```

### Testing the sensing domain socket
Both the kernel module and the sensing client need to agree on the name of the domain socket. The default name is `/var/run/kernel_sensor`.  The client must have root privilege to open the domain socket.

To test the functionality of the domain socket,
1. load the kernel sensing module (see above)
2. run the KernelProbe client with `—echo`

```shell
[mdday@localhost kernel]$ sudo ./KernelProbe.py --echo
connecting to /var/run/kernel_sensor
attempting to open - as output file
response expected: 4.16.14-300.fc28.x86_64

sending "echo"
received "4.16.14-300.fc28.x86_64"
```

A successful echo test is shown above. The echo reply will contain the running kernel version.

### Testing the connect message
The `connect` message is a distinct protocol transaction that is not equivalent to the echo test shown above.

```shell
[mdday@localhost kernel]$ sudo ./KernelProbe.py --connect
connecting to /var/run/kernel_sensor
attempting to open - as output file
sending "{Virtue-protocol-version: 0.1}
"
received "{Virtue-protocol-version: 0.1}
"
```

A successful connect transaction is shown above, using protocol version `0.1`

### Testing the discover message
The `discover` message will reveal the specific sensors and their identifying information (`id`).

(For the field definitions of the `discover` transactions, see `messages.md`.)

```shell
[mdday@localhost kernel]$ sudo ./KernelProbe.py --discover
connecting to /var/run/kernel_sensor
attempting to open - as output file
sending "{Virtue-protocol-version: 0.1, request: [fafc88242e064f819d43e4b3bd321554, discovery, *]}"
{Virtue-protocol-version: 0.1, reply: [fafc88242e064f819d43e4b3bd321554, discovery, ["Kernel Sysfs Probe", "Kernel LSOF Probe", "Kernel PS Probe"]] }
```

Every sensing transaction except `connect` has a uniquely identifying element, the `nonce`, that matches each reply to its originating request.

The `discover` transaction shown above displays the matching `nonce` fields, as well as the `json array` within the response that has the names of each sensor running in the kernel.

### Retrieving sensing data

The record transaction is specified in `messages.md`. Here is a partial record response from the kernel-ps sensor.

```shell
sudo ./KernelProbe.py --records > records.json
connecting to /var/run/kernel_sensor
attempting to open - as output file
sending "{Virtue-protocol-version: 0.1, request: [7c059dcb2e214d41abfef6291514a3d4, records, "Kernel PS Probe"]}"
{Virtue-protocol-version: 0.1, reply: [ 7c059dcb2e214d41abfef6291514a3d4, Kernel PS Probe, kernel-ps 0 systemd 1 0 0 ]}
{Virtue-protocol-version: 0.1, reply: [ 7c059dcb2e214d41abfef6291514a3d4, Kernel PS Probe, kernel-ps 1 kthreadd 2 0 0 ]}
{Virtue-protocol-version: 0.1, reply: [ 7c059dcb2e214d41abfef6291514a3d4, Kernel PS Probe, kernel-ps 2 kworker/0:0H 4 0 0 ]}
{Virtue-protocol-version: 0.1, reply: [ 7c059dcb2e214d41abfef6291514a3d4, Kernel PS Probe, kernel-ps 3 mm_percpu_wq 6 0 0 ]}
…
{Virtue-protocol-version: 0.1, reply: [ 7c059dcb2e214d41abfef6291514a3d4, Kernel PS Probe]}

send_records_message: closing socket
```

## Running the kprobe and uprobe (bcc) Sensors
The `bcc` sensors are user-space programs that install kernel probes and collect probe data. They currently output raw json objects, unorganized by the json sensing protocol. That is planned to change, and they will participate as sensing servers in the the protocol, just as the kernel module sensors now do.

### Current bcc sensors
As of this writing the current bcc sensors include:
1. `bashreadline`
2. `runqlat`
3. `ttysnoop`
4. `tcptop`
5. `filetop`

These five sensors, and more, are available from the [Two Six labs github bcc fork] (https://github.com/twosixlabs/bcc) in the `tools` directory.

Each of the five sensors has been modified in two ways:
* it has been made into a Python class in order to be a base class for a variation of the existing Python sensor wrapper
* it has been made to output sensing data in `json` objects that will fit into the sensing `json` protocol.

### Example output of bashreadline

```shell
[mdday@localhost tools]$ ./bashreadline.py --help
usage: bashreadline.py [-h] [-j]

Trace readline syscalls made by bash

optional arguments:
  -h, --help  show this help message and exit
  -j, --json  output json objects

examples:
        ./bashreadline        # trace all readline syscalls by bash
        ./bashreadline -j     # output json objects
```

```shell
[mdday@localhost tools]$ sudo ./bashreadline.py --json
{"tag": bashreadline, "time": 16:29:11, "pid": 6038, "command": ls -laht }
{"tag": bashreadline, "time": 16:29:20, "pid": 6038, "command": cat /proc/cpuinfo}
```

The `bashreadline` output above shows two command records that were run by other `bash` processes during the short recording interval.

### Example of ttysnoop

```shell
usage: ttysnoop.py [-h] [-C] [-j] device

Snoop output from a pts or tty device, eg, a shell

positional arguments:
  device         path to a tty device (eg, /dev/tty0) or pts number

optional arguments:
  -h, --help     show this help message and exit
  -C, --noclear  don't clear the screen
  -j, --json     output json objects

examples:
        ./ttysnoop /dev/pts/2    # snoop output from /dev/pts/2
        ./ttysnoop 2             # snoop output from /dev/pts/2 (shortcut)
        ./ttysnoop /dev/console  # snoop output from the system console
        ./ttysnoop /dev/tty0     # snoop output from /dev/tty0
```

```shell
[mdday@localhost tools]$ sudo ./ttysnoop.py -C -j /dev/pts/0
{"line": }
{"line": }
{"line": [mdday@localhost kernel]$ }
{"line": ls /etc/systemd/}
{"line": }
{"line": bootchart.conf  journald.conf  network        system       timesyncd.conf  user.conf}
{"line": coredump.conf   logind.conf    resolved.conf  system.conf  user}
{"line": }
{"line": [mdday@localhost kernel]$ }
{"line": p}
{"line": u}
{"line": s}
{"line": h}
{"line": d}
{"line":  }
{"line": /}
{"line": e}
{"line": t}
{"line": c}
{"line": /}
{"line": s}
{"line": y}
{"line": s}
{"line": t}
{"line": e}
{"line": m}
{"line": d}
{"line": }
{"line": /etc/systemd ~/src/savior/control/kernel ~}
{"line": }
{"line": [mdday@localhost systemd]$ }
```

In the above sensing data, some single characters are recorded as pty lines. But it tracks a different tty session and mirrors the input and output on the tty device.

