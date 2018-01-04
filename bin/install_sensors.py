#!/usr/bin/python

import json
import os
import shutil
import sys

"""
Enumerate available sensors, and install them into targets
according to the target sensors.json definition file.

The process for installing a sensor looks like:

 - Destroy existing sensors group directory
 - Create sensors group directory
 - Create sensor directory
 - Copy sensor files into place (sensor.json::files)
 - Create required sub-directories (sensor.json::required_sub_directories)
 - Copy startup scripts into place (sensor.json::startup_script)
 - Copy requirements.txt files into place (sensor.json::requirements_files)
"""


def find_sensors(directory):
    """
    Find all of the sensors defined within a directory, returning a list of
    sensors that looks like:

        [
            {
                "root": "sensor root",
                "sensor": { sensor def},
                "name": <sensor name>
            }
        ]

    :param directory: Root directory to search for sensors.
    :return: List of sensor dictionaries
    """

    sensors = []

    # walk away
    for root, dirs, files in os.walk(directory):
        if "sensor.json" in files:
            sensor = json.load(open(os.path.join(root, "sensor.json"), 'r'))
            sensors.append(
                {
                    "root": root,
                    "sensor": sensor,
                    "name": sensor["name"]
                }
            )

    return sensors


def find_targets(directory):
    """
    Find all of the target images within a directory, returning a list
    of targets that looks like:

        [
            {
                "root": "target root",
                "target": {target def},
                "name": <target name>
            }
        ]

    :param directory:
    :return:
    """

    targets = []

    # walk it
    for root, dirs, files in os.walk(directory):
        if "target.json" in files:
            target = json.load(open(os.path.join(root, "target.json"), "r"))
            targets.append(
                {
                    "root": root,
                    "target": target,
                    "name": target["name"]
                }
            )

    return targets


def validate_target(target, sensors):
    """
    Validate a target definition:

        - all sensors exist
        - directories are defined for:
            - sensors_directory
            - requirements_directory
            - startup_scripts_directory
            - library_directory

    :param target:
    :param sensors:
    :return:
    """

    # invert sensors into hash index
    sensor_idx = {sensor["name"]: True for sensor in sensors}

    errors = []

    for req_sensor in target["target"]["sensors"]:
        if req_sensor not in sensor_idx:
            errors.append("required sensor [%s] does not exist" % (req_sensor,))

    for dir_key in ["sensors_directory", "requirements_directory", "startup_scripts_directory", "library_directory"]:
        if dir_key not in target["target"]:
            errors.append("Required directory definition field missing [%s]" % (dir_key,))
    return errors


def validate_sensor(sensor):
    """
    Make sure all of the files defined by a sensor exist:

        - files[].source
        - startup_script
        - requirements_files

    :param sensor:
    :return:
    """
    errors = []

    if not os.path.isfile(os.path.join(sensor["root"], sensor["sensor"]["startup_script"])):
        errors.append("sensor.json::startup_script does not point to an existing file")

    for req_file in sensor["sensor"]["requirements_files"]:
        if not os.path.isfile(os.path.join(sensor["root"], req_file)):
            errors.append("sensor.json::requirements_file [%s] does not point to an existing file" % (req_file,))

    for src_file in sensor["sensor"]["files"]:
        if not os.path.isfile(os.path.join(sensor["root"], src_file["source"])):
            errors.append("sensor.json::files[]::source [%s] does not point to an existing file" % (src_file["source"],))

    return errors


def prep_target(target):
    """
    Prepare the directories of a target.

    If they exist, remove:

        - sensors_directory
        - requirements_directory
        - startup_scripts_directory

    Create

        - sensors_directory
        - requirements_directory
        - startup_scripts_directory

    :param target:
    :return:
    """

    print "  ~ Preparing directories"
    root = target["root"]
    for dir_key in ["sensors_directory", "requirements_directory", "startup_scripts_directory", "library_directory"]:
        dir = target["target"][dir_key]
        path = os.path.join(root, dir)

        # nuke it if it already exists
        if os.path.exists(path):
            print "    - removing [%s] (%s)" % (dir_key, dir)
            shutil.rmtree(path)

        # create it
        print "    + creating [%s] (%s)" % (dir_key, dir)
        os.makedirs(path)


def install_sensors_in_target(target, sensors, wrapper_dir):
    """
    Install all of the required sensors for the target.

    :param target:
    :param sensors:
    :return:
    """

    # build a quick index of sensors
    sensor_idx = {sensor["name"]: sensor for sensor in sensors}

    # run the install
    print "Installing %d sensors in target [%s]" % (len(target["target"]["sensors"]), target["name"])

    prep_target(target)

    install_sensor_wrapper(target, wrapper_dir)

    for sensor_name in target["target"]["sensors"]:
        install_sensor(target, sensor_idx[sensor_name])


def install_sensor(target, sensor):
    """
    Install a specific sensor into a target.

        - Create sensor directory
        - Copy sensor files into place (sensor.json::files)
        - Create required sub-directories (sensor.json::required_sub_directories)
        - Copy startup scripts into place (sensor.json::startup_script)
        - Copy requirements.txt files into place (sensor.json::requirements_files)

    :param target:
    :param sensor:
    :return:
    """

    root = target["root"]
    sensors_dir = os.path.abspath(os.path.join(root, target["target"]["sensors_directory"]))
    reqs_dir = os.path.abspath(os.path.join(root, target["target"]["requirements_directory"]))
    run_dir = os.path.abspath(os.path.join(root, target["target"]["startup_scripts_directory"]))

    print "  + installing %s (version %s)" % (sensor["name"], sensor["sensor"]["version"])

    # create sensor directory
    sensor_dest_dir = os.path.abspath(os.path.join(sensors_dir, sensor["sensor"]["target_folder"]))
    sensor_src_dir = sensor["root"]

    print "    + run time files"
    os.makedirs(sensor_dest_dir)

    # copy the sensor files
    for file_def in sensor["sensor"]["files"]:
        shutil.copy(
            os.path.abspath(os.path.join(sensor_src_dir, file_def["source"])),
            os.path.abspath(os.path.join(sensor_dest_dir, file_def["dest"]))
        )

    print "    + run time directories"
    for run_sub_dir in sensor["sensor"]["required_sub_directories"]:
        os.makedirs(os.path.abspath(os.path.join(sensor_dest_dir, run_sub_dir)))

    print "    + startup script"
    shutil.copy(
        os.path.abspath(os.path.join(sensor_src_dir, sensor["sensor"]["startup_script"])),
        os.path.abspath(os.path.join(run_dir, sensor["sensor"]["startup_script"]))
    )

    print "    + requirments.txt files"
    for require_txt in sensor["sensor"]["requirements_files"]:
        shutil.copy(
            os.path.abspath(os.path.join(sensor_src_dir, require_txt)),
            os.path.abspath(os.path.join(reqs_dir, require_txt))
        )


def install_sensor_wrapper(target, wrapper_dir):
    """
    Install the sensor wrapper library files into the target.


    :param target:
    :return:
    """

    # define our directories
    root = target["root"]
    reqs_dir = os.path.abspath(os.path.join(root, target["target"]["requirements_directory"]))
    lib_dir = os.path.abspath(os.path.join(root, target["target"]["library_directory"]))

    # install lib files
    print "  + installing Sensor Wrapper library"
    wrapper_dest_dir = os.path.abspath(os.path.join(lib_dir, "sensor_wrapper"))
    os.makedirs(wrapper_dest_dir)
    
    print "    + library files"
    lib_files = ["sensor_wrapper.py", "setup.py"]

    for lib_file in lib_files:
        shutil.copy(
            os.path.abspath(os.path.join(wrapper_dir, lib_file)),
            os.path.abspath(os.path.join(wrapper_dest_dir, lib_file))
        )

    # install requirements.txt file
    print "    + requirements.txt file"
    shutil.copy(
        os.path.abspath(os.path.join(wrapper_dir, "requirements.txt")),
        os.path.abspath(os.path.join(reqs_dir, "requirements.txt"))
    )


if __name__ == "__main__":

    sensor_dir = "./sensors"
    targets_dir = "./targets"
    wrapper_dir = "./sensors/wrapper"

    print "Running install_sensors"
    print "  wrapper(%s)" % (wrapper_dir,)
    print "  sensors(%s)" % (sensor_dir,)
    print "  targets(%s)" % (targets_dir,)
    print ""

    print "Finding Sensors"

    sensors = find_sensors(os.path.abspath(sensor_dir))

    for sensor in sensors:
        print "  + %s (version %s)" % (sensor["name"], sensor["sensor"]["version"])
        errors = validate_sensor(sensor)
        if len(errors) != 0:
            print "    ! errors detected in sensor.json"
            for err in errors:
                print "      - %s" % (err,)
            sys.exit(1)

    print ""
    print "Finding Targets"

    targets = find_targets(targets_dir)

    for target in targets:
        print "  + %s" % (target["name"],)
        errors = validate_target(target, sensors)
        if len(errors) != 0:
            print "    ! errors detected in target.json"
            for err in errors:
                print "      - %s" % (err,)
            sys.exit(1)

    print ""
    for target in targets:
        install_sensors_in_target(target, sensors, wrapper_dir)
