VirtUE/Savior Design and Roadmap

by Mark F. Sanderson

mark.sanderson at twosixlabs dot com

**

Table of Contents

<span id="anchor"></span>Introduction
=====================================

This is the proposed design and road-map document for the Windows x64
VirtUE/Savior sensor/probe system. The high-level description is
contained within the IARPA VirtUE Proposal as authored by Next Century
Technology.

<span id="anchor-1"></span>Design Considerations
================================================

The Windows x64 VirtUE/Savior sensor/probe design, as much as is
practical, will be aligned very close to the current Linux design.
Driver design, implementation, and user/kernel communications will
necessarily be different between these two targets

<span id="anchor-2"></span>Assumptions and Dependencies
-------------------------------------------------------

-   Docker will be used as the sensor software deployment technology
    during development and test
-   Will utilize a variety of Python libraries and packages that should
    function adequately in the Windows user execution environment.
-   In order to ensure safe installation, all kernel-mode software
    (kernel drivers) must be signed utilizing an approved Microsoft code
    signing policy.
-   All development and test will utilize Microsoft's Hyper-V virtual
    machine environment.
-   User level sensor wrappers and API communications will utilize
    currently existing protocols and code where possible
-   Sensors will emit complex JSON objects to represent the VirtUE's
    current state consistent with the current Linux design

<span id="anchor-3"></span>General Constraints
----------------------------------------------

-   This solution will be delivered for the Windows 10 x64 Operating
    System
-   Delivery of sensor functionality, IFFT and other types of automation
    will likely require research
-   Testing will utilize pytest prove basic functionality and regression
    testing
-   The solution will attempt to utilize a Windows version of OpenSSL
    generate certs; if this isn't possible then other libraries/packages
    will looked at to meet this requirement.

<span id="anchor-4"></span>Goals and Guidelines
-----------------------------------------------

-   The Windows VirtUE/Savior sensor system will re-use as much existing
    code, protocols as possible
-   The Windows VirtUE/Savior sensor system will be divided into two
    delivery phases.

    -   Phase one, because of time constraints, will focus on delivery
        user mode sensing built from current Python-based capabilities
    -   Phase two will utilize a Windows kernel-mode driver to interact
        with the operating system to ensure a more complete and accurate
        sensing system.
-   Probes for both phases will hand-off data collected by user mode
    sensors that will then transmit that data to VirtUE/Savior API as is
    also done by the Linux sensor suite.

<span id="anchor-5"></span>Development Methods
----------------------------------------------

-   Phase one development will utilize python 3.6 for Windows x64 along
    with required packages libraries.
-   Code re-use will be the guiding development principal. The windows
    sensor/probe systems will utilize current sensor wrappers, APIs and
    etc as appropriate.
-   An earlier strategy to heavily utilize the pywin32 library for phase
    one user mode sensing was dropped in favor of using ctypes style
    Win32 API calls. The ctypes approach has the advantage of relying
    less on external libraries and utilizing native functionality as
    delivered by Python 3.6.

<span id="anchor-6"></span>Road Map
===================================

<span id="anchor-7"></span>Design Notes
=======================================

Diagrams
--------

### <span id="anchor-8"></span>Sample JSON output

{"VirtUE":

{

"HostName": "value"

"NetworkCards": \[

{

"Name": "eth0"

"IPV4Addresses":

\[

{"Address": "value"}

{"Address": "value"}

\]

"IPV6Addresses":

\[

{"Address": "value"}

{"Address": "value"}

\]

}

\]

"NumberOfProcessors": “value”

"MemoryCapicity": “value”

...

} "Processes": \[

{

"NumberOfThreads": "value"

"WorkingSetPrivate": "value"

"UniqueProcessId": "value"

...

"Handles": \[

{

"Object": "value"

"UniqueProcessId": "value"

"HandleValue": "value"

"ObjectType": "value"

"ObjectName": "value"

...

}

{

"Object": "value"

"UniqueProcessId": "value"

"HandleValue": "value"

"ObjectType": "value"

"ObjectName": "value"

...

}

...

\]

"Threads": \[

{

"KernelTime": "value"

"UserTime": "value"

"CreateTime": "value"

"WaitTime": "value"

"StartAddress": "value"

"ClientId": "value"

...

}

{

"KernelTime": "value"

"UserTime": "value"

"CreateTime": "value"

"WaitTime": "value"

"StartAddress": "value"

"ClientId": "value"

...

}

...

\]

}

\]

}

### <span id="anchor-9"></span>Sensor/Probe Diagram show

+==============================+

| Savior API |

+==============================+

\^

|

v

+==============================+

| Windows Python | sensor\_wrapper |

| Savior Service | Process Sensor | ← User Mode Sensors

| (SCM) | Resource Sensor |

+==============================+

+==============================+

| Windows Operating System | savior.sys | ← Kernel-mode Sensors

+==============================+

###### 
