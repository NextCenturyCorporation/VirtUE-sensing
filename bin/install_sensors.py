#!/usr/bin/python

from argparse import ArgumentParser
import json
import os
import shutil
import sys
import types


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


def find_kmods(directory):
    """
    Find all of the kernel modules within a directory, return a list of
    mods that looks like:

        [
            {
                "root": "kmod root",
                "kernel_module": { kmod def },
                "name": <kmod name>
            }
        ]
    :param directory: Root directory to search for kernel modules
    :return:
    """

    kmods = []

    # walk the line
    for root, dirs, files in os.walk(directory):
        if "kernel_module.json" in files:
            kmod = json.load(open(os.path.join(root, "kernel_module.json"), "r"))
            kmods.append(
                {
                    "root": root,
                    "kernel_module": kmod,
                    "name": kmod["name"]
                }
            )
    return kmods


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


def validate_kmod(kmod):
    """
    validate the definition of a kernel module:

        Directories:
            - include

        Files:
            - install

        Is List:
            - apt-get

        exists:
            - name
            - version
            - target_folder

    :param kmod:
    :return:
    """
    errors = []

    # validate that the include directories exist
    for include_dir in kmod["kernel_module"]["include"]:
        if not os.path.exists(os.path.join(kmod["root"], include_dir)):
            errors.append("kernel_module.json::include does not point to an existing file or directory")

    # validate that the install files are real
    for install_file in kmod["kernel_module"]["install"]:
        if not os.path.isfile(os.path.join(kmod["root"], install_file)):
            errors.append("kernel_module.json::install entry does not point to an existing file")

    # keys must exist
    for k in ["apt-get", "name", "version", "target_folder"]:
        if k not in kmod["kernel_module"]:
            errors.append("kernel_module.json::%s is a required key" % (k,))

    # the apt-get key needs to be a list
    if "apt-get" not in kmod["kernel_module"] or type(kmod["kernel_module"]["apt-get"]) != types.ListType:
        errors.append("kernel_module.json::apt-get needs to be a list")

    return errors


def validate_target(target, sensors, kmods):
    """
    Validate a target definition:

        - all sensors exist
        - all kernel_modules exist
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
    kmod_idx = {kmod["name"]: True for kmod in kmods}

    errors = []

    for req_sensor in target["target"]["sensors"]:
        if req_sensor not in sensor_idx:
            errors.append("required sensor [%s] does not exist" % (req_sensor,))

    for req_kmod in target["target"]["kernel_modules"]:
        if req_kmod not in kmod_idx:
            errors.append("required kernel module [%s] does not exist" % (req_kmod,))

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

    Make sure the following fields are defined as lists (empty lists are fine):

        - required_sub_directories
        - apt-get

    Make sure the following fields are defined:

        - name
        - version
        - target_folder

    :param sensor:
    :return:
    """
    errors = []

    # files exist
    if not os.path.isfile(os.path.join(sensor["root"], sensor["sensor"]["startup_script"])):
        errors.append("sensor.json::startup_script does not point to an existing file")

    for req_file in sensor["sensor"]["requirements_files"]:
        if not os.path.isfile(os.path.join(sensor["root"], req_file)):
            errors.append("sensor.json::requirements_file [%s] does not point to an existing file" % (req_file,))

    for src_file in sensor["sensor"]["files"]:
        if not os.path.isfile(os.path.join(sensor["root"], src_file["source"])):
            errors.append("sensor.json::files[]::source [%s] does not point to an existing file" % (src_file["source"],))

    # lists exist
    list_keys = ["required_sub_directories", "apt-get"]
    for list_key in list_keys:
        if list_key not in sensor["sensor"]:
            errors.append("sensor.json::%s required key is missing" % (list_key,))
        else:
            if type(sensor["sensor"][list_key]) != types.ListType:
                errors.append("sensor.json::%s must be a list" % (list_key,))

    # fields are defined
    field_keys = ["name", "version", "target_folder"]
    for field_key in field_keys:
        if field_key not in sensor["sensor"]:
            errors.append("sensor.json::%s required key is missing" % (field_key,))

    return errors


def prep_target(target):
    """
    Prepare the directories of a target.

    If they exist, remove:

        - sensors_directory
        - requirements_directory
        - startup_scripts_directory
        - kernel_mod_directory

    Create

        - sensors_directory
        - requirements_directory
        - startup_scripts_directory
        - kernel_mod_directory

    :param target:
    :return:
    """

    print "  ~ Preparing directories"
    root = target["root"]
    for dir_key in ["sensors_directory", "requirements_directory", "startup_scripts_directory", "library_directory", "kernel_mod_directory"]:
        dir = target["target"][dir_key]
        path = os.path.join(root, dir)

        # nuke it if it already exists
        if os.path.exists(path):
            print "    - removing [%s] (%s)" % (dir_key, dir)
            shutil.rmtree(path)

        # create it
        print "    + creating [%s] (%s)" % (dir_key, dir)
        os.makedirs(path)


def install_sensors_in_target(target, kmods, sensors, wrapper_dir):
    """
    Install all of the required sensors for the target.

    :param target:
    :param sensors:
    :return:
    """

    # build a quick index of sensors
    sensor_idx = {sensor["name"]: sensor for sensor in sensors}
    kmod_idx = {kmod["name"]: kmod for kmod in kmods}

    # run the install
    print "Installing %d sensors in target [%s]" % (len(target["target"]["sensors"]), target["name"])

    # directory prep
    prep_target(target)

    # install the kernel module data
    for kmod_name in target["target"]["kernel_modules"]:
        install_kernel_module(target, kmod_idx[kmod_name])

    # support libraries
    install_sensor_wrapper(target, wrapper_dir)

    # individual sensors
    for sensor_name in target["target"]["sensors"]:
        install_sensor(target, sensor_idx[sensor_name])

    # post-install files, like global requirements file
    create_requirements_master(target)

    # support library install file
    create_support_library_install_script(target)

    # kernel module install file
    create_kernel_module_install_script(target, kmods)

    # single point sensor startup script
    create_sensor_startup_master(target)

    # collate apt-get install requirements, and write out to sensor_libraries/apt-get-support.sh
    create_apt_get_script(target, sensors, kmods)


def create_apt_get_script(target, sensors, kmods):
    """
    Collate all of the apt-get requirements, and write them out to a file that
    can be called from the target Dockerfile

    :param target:
    :param sensors:
    :return:
    """
    print "  + Finding apt-get requirements for installed sensors"

    # directory were we'll create the apt_get_install.sh script
    lib_dir = os.path.abspath(os.path.join(target["root"], target["target"]["library_directory"]))

    # libraries we need
    apt_libs = []

    # inverted sensor index
    sensor_idx = {sensor["name"]: sensor for sensor in sensors}
    kmod_idx = {kmod["name"]: kmod for kmod in kmods}

    for sensor_name in target["target"]["sensors"]:
        apt_libs += sensor_idx[sensor_name]["sensor"]["apt-get"]

    for kmod_name in target["target"]["kernel_modules"]:
        apt_libs += kmod_idx[kmod_name]["kernel_module"]["apt-get"]

    # normalize the list and remove duplicates
    apt_libs = list(set(apt_libs))

    print "    = Found %d required libraries" % (len(apt_libs),)

    # even if we don't have libraries we need a script, because it's just about impossible
    # to usefully check for the presence of the install script in the Dockerfile itself.
    with open(os.path.abspath(os.path.join(lib_dir, "apt_get_install.sh")), "w") as agi_script:
        agi_script.write("#!/bin/bash\n")

        if len(apt_libs) > 0:
            agi_script.write("apt-get install -y %s\n" % (" ".join(apt_libs),))


def create_sensor_startup_master(target):
    """
    Create a single script that spins up all of the sensor startup
    scripts, making them run nohup/daemon.

    :param target:
    :return:
    """
    print "  + Building sensor startup master script"

    start_dir = os.path.abspath(os.path.join(target["root"], target["target"]["startup_scripts_directory"]))

    scripts = os.listdir(start_dir)
    with open(os.path.abspath(os.path.join(start_dir, "run_sensors.sh")), "w") as master_script:
        master_script.write("#!/bin/bash\n")

        for script in scripts:
            master_script.write("/opt/sensor_startup/%s &\n" % (script,))

    print "    + %d startup scripts added" % (len(scripts),)


def create_kernel_module_install_script(target, kmods):
    """
    Create a build script for the kernel modules.

        1. Scan for install script files in the selected kernel modules
        2. create script to move into directories and build

    :param target:
    :param kmods:
    :return:
    """
    print "  + Building kernel module install script"

    kmod_root_dir = os.path.abspath(os.path.join(target["root"], target["target"]["kernel_mod_directory"]))

    # find related kernel mods
    kmod_idx = {kmod["name"]: kmod for kmod in kmods}

    build_steps = []

    for req_kmod in target["target"]["kernel_modules"]:
        kmod = kmod_idx[req_kmod]["kernel_module"]

        # what's the relative directory
        mod_dir = os.path.abspath(os.path.join(kmod_root_dir, kmod["target_folder"]))

        # build out the install scripts
        for inst_file in kmod["install"]:

            # get the install file
            inst_file = os.path.abspath(os.path.join(mod_dir, inst_file))

            # find the relative file
            inst_run_file = os.path.basename(inst_file)
            inst_rel_dir = os.path.relpath(os.path.dirname(inst_file), kmod_root_dir)

            # relative up path
            up_rel_dir = os.path.relpath(kmod_root_dir, os.path.dirname(inst_file))

            # add the actual build step
            build_steps.append("cd %s\nbash %s\ncd %s\n" % (inst_rel_dir, inst_run_file, up_rel_dir))

    # write out the install file
    print "  + Writing install file"
    with open(os.path.join(kmod_root_dir, "install.sh"), "w") as kmod_installer:
        kmod_installer.write("#!/bin/bash\n")

        for build_step in build_steps:
            kmod_installer.write(build_step)


def create_support_library_install_script(target):
    """
    Build a script in the sensor_libraries directory that runs
    any install steps we need.

        1. Scan for setup.py files, and build out pip install for all of them

    :param target:
    :return:
    """

    print "  + Building support library install script"

    lib_dir = os.path.abspath(os.path.join(target["root"], target["target"]["library_directory"]))

    # find any pip install targets
    print "    % Scanning for pip install targets"
    pip_installs = []

    for root, dirs, files in os.walk(lib_dir):
        if "setup.py" in files:
            pip_installs.append(os.path.relpath(root, lib_dir))

    print "    + Writing install script"

    with open(os.path.abspath(os.path.join(lib_dir, "install.sh")), "w") as installer:
        for pip_install in pip_installs:
            installer.write("pip install ./%s\n" % (pip_install,))


def create_requirements_master(target):
    """
    Scan the requirements files in the target's requirements directory and
    construct the `requirements_master.txt` which points to each of the
    other requirements files, with contents like:

        -r req1.txt
        -r req2.txt

    :param target: Install target
    :return:
    """
    print "  + Creating requirements_master.txt to consolidate required libraries"

    req_dir = os.path.abspath(os.path.join(target["root"], target["target"]["requirements_directory"]))

    # scan for requirements files
    files = os.listdir(req_dir)

    # now build the contents of our master file
    with open(os.path.abspath(os.path.join(req_dir, "requirements_master.txt")), "w") as req_file:
        for file in files:
            req_file.write("-r %s\n" % (file,))


def install_kernel_module(target, kmod):
    """
    Install a specif kernel module into a target.

        - Create the module directory
        - Copy included files into place (kernel_module.json::include)


    :param target:
    :param kmod:
    :return:
    """
    root = target["root"]
    kmod_dir = os.path.abspath(os.path.join(root, target["target"]["kernel_mod_directory"]))

    print "  + installing %s (version %s)" % (kmod["name"], kmod["kernel_module"]["version"])

    kmod_root_dir = os.path.abspath(os.path.join(kmod_dir, kmod["kernel_module"]["target_folder"]))

    # walk through the includes and copy them into place
    for inc_path in kmod["kernel_module"]["include"]:
        kmod_src_dir = os.path.abspath(os.path.join(kmod["root"], inc_path))
        kmod_dst_dir = os.path.abspath(os.path.join(kmod_root_dir, inc_path))

        print "    ~ %s" % (inc_path,)
        # os.makedirs(kmod_dst_dir)
        shutil.copytree(kmod_src_dir, kmod_dst_dir)


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

        # we need to create a simple empty dot file in a directory otherwise it's not recorded
        # in git, which messes up other installations, because an empty directory isn't saved,
        # and other systems generating targets will be missing things like 'certs' directories
        # which then causes everything to crash
        with open(os.path.join(os.path.join(sensor_dest_dir, run_sub_dir), ".empty"), "w") as dotfile:
            dotfile.write("\n")

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
        os.path.abspath(os.path.join(wrapper_dir, "sensor_wrapper_requirements.txt")),
        os.path.abspath(os.path.join(reqs_dir, "sensor_wrapper_requirements.txt"))
    )


def options():
    """
    Let's parse our CLI options...

    :return:
    """
    parser = ArgumentParser("Install sensors, modules, and supporting files into build targets")

    # are we installing? are we listing?
    parser.add_argument("mode", metavar="M", nargs='?', default="install", help="Top level interaction",
                        choices=["install", "list"])

    # skip certain components
    parser.add_argument("--skip-sensor", dest="skip_sensors", default=[], nargs="*", help="Sensors to skip")
    parser.add_argument("--skip-module", dest="skip_modules", default=[], nargs="*", help="Modules to skip")
    parser.add_argument("--skip-target", dest="skip_targets", default=[], nargs="*", help="Targets to skip")

    return parser.parse_args()


if __name__ == "__main__":

    opts = options()

    sensor_dir = "./sensors"
    targets_dir = "./targets"
    wrapper_dir = "./sensors/wrapper"
    kernel_dir = "./"

    print "Running install_sensors"
    print "  wrapper(%s)" % (wrapper_dir,)
    print "  sensors(%s)" % (sensor_dir,)
    print "  targets(%s)" % (targets_dir,)
    print "  kernel_mods(%s)" % (kernel_dir,)
    print ""

    print "Finding Sensors"

    # find all of the sensors on our path
    sensors = find_sensors(os.path.abspath(sensor_dir))

    # as we iterate and validate our sensors, we build up a list of sensors
    # we're including in further actions. This can be affected by the --skip-sensor
    # flag
    sensor_skip_set = set(opts.skip_sensors)
    included_sensors = []

    for sensor in sensors:

        if sensor["name"] in sensor_skip_set:
            print "  # %s (version %s) - skipped" % (sensor["name"], sensor["sensor"]["version"])
            continue

        print "  + %s (version %s)" % (sensor["name"], sensor["sensor"]["version"])
        errors = validate_sensor(sensor)
        if len(errors) != 0:
            print "    ! errors detected in sensor.json"
            for err in errors:
                print "      - %s" % (err,)
            sys.exit(1)

        included_sensors.append(sensor)

    print ""
    print "Finding kernel modules"

    # find all of the sensors on our path
    kmods = find_kmods(kernel_dir)

    # we can elide modules while we iterate, as controlled by the --skip-module flag
    skip_module_set = set(opts.skip_modules)
    included_modules = []

    for kmod in kmods:

        if kmod["name"] in skip_module_set:
            print "  # %s (version %s) - skipped" % (kmod["name"], kmod["kernel_module"]["version"])
            continue

        print "  + %s (version %s)" % (kmod["name"], kmod["kernel_module"]["version"])
        errors = validate_kmod(kmod)
        if len(errors) != 0:
            print "    ! errors detected in kernel_module.json"
            for err in errors:
                print "      - %s" % (err,)
            sys.exit(1)
        included_modules.append(kmod)

    print ""
    print "Finding Targets"

    # find all of the targets on the system
    targets = find_targets(targets_dir)

    # like modules and sensors, we can skip targets, as specified with the --skip-target flag
    skip_target_set = set(opts.skip_targets)
    included_targets = []

    for target in targets:

        if target["name"] in skip_target_set:
            print "  # %s - skipped" % (target["name"],)
            continue

        print "  + %s" % (target["name"],)
        errors = validate_target(target, included_sensors, included_modules)
        if len(errors) != 0:
            print "    ! errors detected in target.json"
            for err in errors:
                print "      - %s" % (err,)
            sys.exit(1)

        included_targets.append(target)

    print ""

    if opts.mode == "install":
        print "Installing components in Targets"
        for target in included_targets:
            install_sensors_in_target(target, included_modules, included_sensors, wrapper_dir)
