#!/usr/bin/python

"""
Find and install sensor configurations into the Sensing API.
"""
from argparse import ArgumentParser
import json
import os
import requests


def find_configuration_collections(base_dir):
    """
    Find the top level "sensor_configurations.json" files that describe
    configuration sets. Save the contents of the configuration top level
    JSON, as well as load the contents (unparsed) pointed to by each
    configuration entry.

    A sensor_configurations.json file looks like:

        {
          "name": "lsof",
          "context": "virtue",
          "os": "linux",
          "description": "Sensing running processes based on the user space `ps` command line tool",
          "configurations": [
            {
              "version": "latest",
              "level": "default",
              "format": "json",
              "source": "ps_config_default_latest.json",
              "description": "Default configuration includes moderately frequent monitoring"
            },
            ...
          ]
        }

    Where the `name`, `context`, `os`, and `description` at the root of the object
    describe what the API considers a Component, and each entry in the `configurations`
    list describes a Configuration.

    The `source` of each configuration should be path relative to the sensor_configurations.json
    file, and will be loaded and stuffed into the `configuration_content` entry of
    the configuration structure when scanned by this method.

    :param base_dir:
    :return: List of sensor configuration objects, each a list of sensors
    """

    sconfs = []

    # walk looking for sensor_configurations.json files
    for root, dirs, files in os.walk(os.path.abspath(base_dir)):

        # do we have a sensors_configuration file?
        if "sensor_configurations.json" in files:
            sconfs.append(load_configurations(root))

    return sconfs


def load_configurations(root):
    """
    Given a root directory containing a sensor configurations JSON file,
    load it up and load the config content
    :param root: Root directory containing a sensor_configurations.json file
    :return:
    """

    conf = json.load(open(os.path.join(root, "sensor_configurations.json"), 'r'))

    for variant in conf["configurations"]:

        # figure out the path
        variant_path = os.path.abspath(os.path.join(root, variant["source"]))

        # load it up
        variant["configuration_content"] = open(variant_path, "r").read()

    return conf


def install_component(comp):
    """
    Given the top level of a configuration, which describes a component, make
    sure the component is installed - if it isn't installed, install it.

    :param comp:
    :return:
    """

    # does it exist?
    component = {
        "name": comp["name"],
        "os": comp["os"],
        "context": comp["context"]
    }

    res = requests.get("http://localhost:17141/api/v1/configuration/component", params=component)

    if res.status_code != 200:
        print "    Encountered an error checking for a component status_code(%d)" % (res.status_code,)
        print "    ", res.text
        return "error"

    # if we get a component back, we're good
    if "component" in res.json():
        return "exists"

    # ok - install we will
    component["description"] = comp["description"]

    res = requests.put("http://localhost:17141/api/v1/configuration/component/create", json=component)

    if res.status_code == 200:
        print "    Component name(%(name)s) os(%(os)s) context(%(context)s) created successfully" % component
        return "created"
    else:
        print "    Encountered an error creating component name(%(name)s) os(%(os)s) context(%(context)s)" % component
        print "       = status_code(%d)" % (res.status_code,)
        print "       ", res.text
        return "error"


def install_configuration(component, configuration):
    """
    Given a top level component def and the configuration, check if one already exists, and if
    not, install.

    :param component:
    :param configuration:
    :return:
    """

    comp_filter = {
        "name": component["name"],
        "os": component["os"],
        "context": component["context"]
    }

    conf_filter = {
        "level": configuration["level"],
        "version": configuration["version"]
    }

    conf_filter.update(comp_filter)

    # does it exist?
    res = requests.get("http://localhost:17141/api/v1/configuration/component/configuration", params=conf_filter)

    if res.status_code not in [200, 400]:
        print "    Encountered an error checking for a configuration status_code(%d)" % (res.status_code,)
        print "      ", res.text
        return "error"

    # if we got a configuration in the results we're all set
    if "configuration" in res.json():
        return "exists"

    # now we need to install
    conf_filter["configuration"] = configuration["configuration_content"]
    conf_filter["description"] = configuration["description"]
    conf_filter["format"] = configuration["format"]

    res = requests.put("http://localhost:17141/api/v1/configuration/component/configuration/create", json=conf_filter)

    if res.status_code == 200:
        print "    Configuration level(%(level)s) version(%(version)s) installed" % conf_filter
        return "created"
    else:
        print "    Encountered an error install configuration level(%(level)s) version(%(version)s)" % conf_filter
        print "      = status_code(%d)" % (res.status_code,)
        print "      ", res.text
        return "error"


def install(config):
    """
    Run through installing a component and its configurations.

    :param config:
    :return:
    """
    print "Installing os(%(os)s) context(%(context)s) name(%(name)s)" % config
    print "  = %d configuration variants" % (len(config["configurations"]),)

    print "  %% installing component name(%(name)s)" % config
    res = install_component(config)
    print "    = %s" % (res,)

    for configuration in config["configurations"]:
        print "  %% install configuration level(%(level)s) version(%(version)s)" % configuration
        res = install_configuration(config, configuration)
        print "    = %s" % (res,)


if __name__ == "__main__":

    configs = find_configuration_collections("./")

    for config in configs:
        install(config)