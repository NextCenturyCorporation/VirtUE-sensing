The SAVIOR Windows VirtUE Sensor Service.

## Summary

These Python modules facilitate the transfer of sensing events from
the WinVirtUE kernel driver (managed by the "WinVirtUE" service) to
the sensing API. They implement a separate Windows service, which is
to be installed and started automatically on bootup.

## Installation

The service is installed as a part of the standard Windows installation, i.e. via

```Cmd
bin\windows-build.bat
```

or

```Cmd
bin\windows-update.bat
```

If you'd like to install manually, you can run this command from `C:\` :

```Cmd
python WinVirtUE\service_winvirtue.py --startup=auto install
sc config "WinVirtUE Service" depend=WinVirtUE
```

## Usage

To use the service, you can either reboot the machine, or else you can
start the service manually:


```Cmd
python WinVirtUE\service_winvirtue.py start
```

or

```Cmd
net start "WinVirtUE Service"
```

## Misc

The registry keys of interest for this service are:

HKLM\System\CCS\Services\WinVirtUE Service
HKLM\System\CCS\Services\WinVirtUE
