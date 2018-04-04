#!"c:\program files\python\python36\python.exe"
import datetime
import json
import wmi
import re
import os
from curio import sleep
import subprocess
from sensor_wrapper import SensorWrapper, report_on_file, which_file

__VERSION__ = "1.20180404"

"""
Sensor that sends process and thread deletion and creation
"""

async def _thread_monitor(message_stub, config, message_queue, notification_type):
    """
    Listen for WMI Thread Creation/Deletion Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :param notification_type: creation or deletion
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)
    print("    $ repeat-interval = %d" % (repeat_delay,))
    c = wmi.WMI()

    while True:

        thread_notify = c.watch_for(notification_type=notification_type, wmi_class="Win32_Process")

        logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": {
                "CSName": thread_notify.CSName,
                "ElapsedTime": thread_notify.ElapsedTime,
                "Handle": thread_notify.Handle,
                "KernelModeTime": thread_notify.KernelModeTime,
                "OSName": thread_notify.OSName,
                "Priority": thread_notify.Priority,
                "PriorityBase": thread_notify.PriorityBase,
                "ProcessHandle": thread_notify.ProcessHandle,
                "StartAddress": thread_notify.StartAddress,
                "ThreadState": thread_notify.ThreadState,
                "ThreadWaitReason": thread_notify.ThreadWaitReason,
                "UserModeTime": thread_notify.UserModeTime,
            }
        }

        logmsg.update(message_stub)

        await message_queue.put(json.dumps(logmsg))

        # sleep
        repeat_delay = config.get("repeat-interval", 15)
        await sleep(repeat_delay)


async def _process_monitor(message_stub, config, message_queue, notification_type):
    """
    Listen for WMI Process Creation/Deletion Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :param notification_type: creation or deletion
    :return: None
    """
    repeat_delay = config.get("repeat-interval", 15)
    print("    $ repeat-interval = %d" % (repeat_delay,))
    c = wmi.WMI()

    while True:

        process_notify = c.watch_for(notification_type=notification_type, wmi_class="Win32_Process")

        logmsg = {
            "timestamp": datetime.datetime.now().isoformat(),
            "level": "info",
            "message": {
                "Caption": process_notify.Caption,
                "CommandLine": process_notify.CommandLine,
                "CreationDate": process_notify.CreationDate,
                "CSName": process_notify.CSName,
                "Description": process_notify.Description,
                "ExecutablePath": process_notify.ExecutablePath,
                "Handle": process_notify.Handle,
                "HandleCount": process_notify.HandleCount,
                "KernelModeTime": process_notify.KernelModeTime,
                "MaximumWorkingSetSize": process_notify.MaximumWorkingSetSize,
                "MinimumWorkingSetSize": process_notify.MinimumWorkingSetSize,
                "Name": process_notify.Name,
                "OSName": process_notify.OSName,
                "OtherOperationCount": process_notify.OtherOperationCount,
                "OtherTransferCount": process_notify.OtherTransferCount,
                "PageFaults": process_notify.PageFaults,
                "PageFileUsage": process_notify.PageFileUsage,
                "ParentProcessId": process_notify.ParentProcessId,
                "PeakPageFileUsage": process_notify.PeakPageFileUsage,
                "PeakVirtualSize": process_notify.PeakVirtualSize,
                "PeakWorkingSetSize": process_notify.PeakWorkingSetSize,
                "Priority": process_notify.Priority,
                "PrivatePageCount": process_notify.PrivatePageCount,
                "ProcessId": process_notify.ProcessId,
                "QuotaNonPagedPoolUsage": process_notify.QuotaNonPagedPoolUsage,
                "QuotaPagedPoolUsage": process_notify.QuotaPagedPoolUsage,
                "QuotaPeakNonPagedPoolUsage": process_notify.QuotaPeakNonPagedPoolUsage,
                "QuotaPeakPagedPoolUsage": process_notify.QuotaPeakPagedPoolUsage,
                "ReadOperationCount": process_notify.ReadOperationCount,
                "ReadTransferCount": process_notify.ReadTransferCount,
                "SessionId": process_notify.SessionId ,
                "ThreadCount": process_notify.ThreadCount,
                "UserModeTime": process_notify.UserModeTime,
                "VirtualSize": process_notify.VirtualSize,
                "WindowsVersion": process_notify.WindowsVersion,
                "WorkingSetSize": process_notify.WorkingSetSize,
                "WriteOperationCount": process_notify.WriteOperationCount,
                "WriteTransferCount": process_notify.WriteTransferCount
            }
        }

        logmsg.update(message_stub)

        await message_queue.put(json.dumps(logmsg))

        # sleep
        repeat_delay = config.get("repeat-interval", 15)
        await sleep(repeat_delay)

async def process_deletion(message_stub, config, message_queue):
    """
    Listen for WMI Process Deletion Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    print(" ::starting process deletion monitor\n")
    _process_monitor(message_stub, config, message_queue, "Deletion")

async def process_creation(message_stub, config, message_queue):
    """
    Listen for WMI Process Creation Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    print(" ::starting process creation monitor\n")
    _process_monitor(message_stub, config, message_queue, "Creation")

async def thread_deletion(message_stub, config, message_queue):
    """
    Listen for WMI Thread Deletion Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    print(" ::starting process deletion monitor\n")
    _thread_monitor(message_stub, config, message_queue, "Deletion")

async def thread_creation(message_stub, config, message_queue):
    """
    Listen for WMI Thread Creation Events and report them

    :param message_stub: Fields that we need to interpolate into every message
    :param config: Configuration from our sensor, from the Sensing API
    :param message_queue: Shared Queue for messages
    :return: None
    """
    print(" ::starting process creation monitor\n")
    _thread_monitor(message_stub, config, message_queue, "Creation")

if __name__ == "__main__":

    wrapper = SensorWrapper("processlist", [process_creation, process_deletion, thread_creation, thread_deletion])
    wrapper.start()
