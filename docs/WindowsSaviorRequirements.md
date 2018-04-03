Software Requirements Specification

for

Savior

Version 1.0

Prepared by Mark Sanderson

TwoSix Labs

March 1, 2018

<span id="anchor"></span>Table of Contents

<span id="anchor-1"></span>Revision History

<span id="anchor-2"></span>Introduction
=======================================

<span id="anchor-3"></span>Purpose
----------------------------------

Windows User and Kernel sensing, targeting the Windows 10 x64
environment

<span id="anchor-4"></span>Document Conventions
-----------------------------------------------

This document does not contain any unique or unusual documentation
conventions.

<span id="anchor-5"></span>Intended Audience and Reading Suggestions
--------------------------------------------------------------------

This documents intended audience is virtue/savior developers, architects
and program managers

<span id="anchor-6"></span>Product Scope
----------------------------------------

Windows Kernel Sensing Research

<span id="anchor-7"></span>References
-------------------------------------

1.  TwoSixLabs Research: <https://github.com/twosixlabs/savior>
2.  Savior Documentation:
    <https://github.com/twosixlabs/savior/tree/master/docs>

<span id="anchor-8"></span>Overall Description
==============================================

<span id="anchor-9"></span>Product Perspective
----------------------------------------------

The Windows Savior package will be a new savior sensor/probe package
designed specifically to target the Windows 10 x64 environment. This
package extends the current sensoring capabilities of the VirtUE/Savior
system.

<span id="anchor-10"></span>Product Functions
---------------------------------------------

-   Initial goal will be to emulate as much as practical the current
    Linux based sensor system. This will include both process lists (ps)
    and open file handles (lsof) to be delivered as sensor data to the
    controlling api.
-   For post mid-term deployments, VirtUE/Savior will utilize a Windows
    Mini-Port File System Filter installed on the target to retrieve
    data unavailable through user space. This data may include, but not
    be limited to:

    -   -   Objects associated with the kernel (PID 4) that can show
            file, registry keys and etc that are managed by the kernel
            itself
        -   Objects associated with protected and critical processes
        -   Behavioral analysis utilizing IFTTT or similar approach
        -   Detecting unusual driver/object utilization that might
            possibly detect key loggers, network taps and etc.

<span id="anchor-11"></span>User Classes and Characteristics
------------------------------------------------------------

The design approach will be similar to if not an exact copy of the
current approach. This approach implements a design: API → Sensor(s) →
Probes. Where the API is the means by which sensors communicate results.
The sensors will control/manage some 'n' number of probes. Probes
capture subject matter data and hand it off to probes. Probes are
responsible for communicating this data back to the API.

<span id="anchor-12"></span>Operating Environment
-------------------------------------------------

-   Windows 10 x64 1709
-   Python 3.6.x

<span id="anchor-13"></span>Design and Implementation Constraints
-----------------------------------------------------------------

-   Targeted specifically for Windows 10 x64

<span id="anchor-14"></span>User Documentation
----------------------------------------------

-   Savior walk-through:
    savior/blob/master/docs/walkthrough\_2018\_01\_10.pdf

<span id="anchor-15"></span>Assumptions and Dependencies
--------------------------------------------------------

### Development

-   Windows 10 x64
-   Docker

<!-- -->

-   Visual Studio Pro
-   Hyper-V
-   python savior API
-   python sensor wrapper

### Deployment

-   Windows 10 x64
-   Ability to load a custom driver on Windows 10 x64


