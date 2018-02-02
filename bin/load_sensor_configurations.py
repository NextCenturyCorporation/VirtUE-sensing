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


def construct_api_url(opts, path, secure=False):
    """
    Put together the API URI from our command line configuration
    :param opts:
    :param path:
    :return:
    """
    # setup the host
    host = opts.api_host

    if not host.startswith("http"):

        if secure:
            host = "https://%s:%d" % (host, opts.api_port_https)
        else:
            host = "http://%s:%d" % (host, opts.api_port_http)

    # setup the full uri
    return "%s/api/%s%s" % (host, opts.api_version, path)


def install_component(opts, comp):
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

    # try and retrieve this component
    uri = construct_api_url(opts, "/configuration/component")
    res = requests.get(uri, params=component)

    if res.status_code not in [200, 400]:
        print "    Encountered an error checking for a component status_code(%d)" % (res.status_code,)
        print "    ", res.text
        return "error"

    # if we get a component back, we're good
    mode = "create"
    if "component" in res.json():
        if not opts.update:
            return "exists"
        else:
            mode = "update"
            print "    == exists / updating"

    # ok - install we will
    component["description"] = comp["description"]

    uri = construct_api_url(opts, "/configuration/component/%s" % (mode,))
    res = requests.put(uri, json=component)

    if res.status_code == 200:
        print "    Component name(%(name)s) os(%(os)s) context(%(context)s) installed successfully" % component
        return "%sd" % (mode,)
    else:
        print "    Encountered an error creating component name(%(name)s) os(%(os)s) context(%(context)s)" % component
        print "       = status_code(%d)" % (res.status_code,)
        print "       ", res.text
        return "error"


def install_configuration(opts, component, configuration):
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

    # does it exist already?
    uri = construct_api_url(opts, "/configuration/component/configuration")
    res = requests.get(uri, params=conf_filter)

    if res.status_code not in [200, 400]:
        print "    Encountered an error checking for a configuration status_code(%d)" % (res.status_code,)
        print "      ", res.text
        return "error"

    # if we got a configuration in the results we're all set
    mode = "create"
    if "configuration" in res.json():
        if not opts.update:
            return "exists"
        else:
            mode = "update"
            print "    == exists / updating"


    # now we need to install
    conf_filter["configuration"] = configuration["configuration_content"]
    conf_filter["description"] = configuration["description"]
    conf_filter["format"] = configuration["format"]

    uri = construct_api_url(opts, "/configuration/component/configuration/%s" % (mode,))
    res = requests.put(uri, json=conf_filter)

    if res.status_code == 200:
        print "    Configuration level(%(level)s) version(%(version)s) installed successfully" % conf_filter
        return "%sd" % (mode,)
    else:
        print "    Encountered an error install configuration level(%(level)s) version(%(version)s)" % conf_filter
        print "      = status_code(%d)" % (res.status_code,)
        print "      ", res.text
        return "error"


def install(opts, config):
    """
    Run through installing a component and its configurations.

    :param config:
    :return:
    """
    print "Installing os(%(os)s) context(%(context)s) name(%(name)s)" % config
    print "  = %d configuration variants" % (len(config["configurations"]),)

    print "  %% installing component name(%(name)s)" % config
    res = install_component(opts, config)
    print "    = %s" % (res,)

    for configuration in config["configurations"]:
        print "  %% install configuration level(%(level)s) version(%(version)s)" % configuration
        res = install_configuration(opts, config, configuration)
        print "    = %s" % (res,)


def list_components(opts):
    """
    Build out each component, as well as the configurations.

    :param opts:
    :return:
    """

    # pull the list of all of the components
    uri = construct_api_url(opts, "/configuration/components")
    res = requests.get(uri)

    if res.status_code != 200:
        print "!! Got an error from the API: %s" % (res.text,)
        return []

    # now let's build out our component tree, starting with iteration
    # over the components just returned.
    components = []

    for comp in res.json()["components"]:

        # let's get the configurations for this components (the component data
        # is included as part of the configuration data, so we can just keep
        # this around instead of building a new compound object
        components.append(list_configurations(opts, comp))

    return components


def list_configurations(opts, component):
    """
    Build out a list of configurations for a given component.

    :param opts:
    :param component:
    :return:
    """

    uri = construct_api_url(opts, "/configuration/component/configurations")
    params = {
        "os": component["os"],
        "context": component["context"],
        "name": component["name"]
    }
    res = requests.get(uri, params=params)

    if res.status_code != 200:
        print "!! got an error retrieving configurations for name(%(name)s) os(%(os)s) context(%(context)s)" % params
        print res.text
        return []

    parsed = res.json()
    comp = parsed["component"]
    comp["configurations"] = parsed["configurations"]
    return comp


def list_action(opts):
    """
    Build out a tree list of all of the current configurations.

    :param opts:
    :return:
    """
    sort_order = {"off": 1, "low": 2, "default": 3, "high": 4, "adversarial": 5}

    components = list_components(opts)

    for comp in components:

        print "%s" % (comp["name"],)
        print "  %% os(%(os)s) context(%(context)s)" % comp

        # we're going to sort the configurations to make more sense of them
        configs = sorted(comp["configurations"], key=lambda conf: sort_order.get(conf["level"], 0))

        for conf in configs:
            print "    [%(level)15s / %(version)-20s] - format(%(format)8s) - last updated_at(%(updated_at)s)" % conf


def options():

    parser = ArgumentParser("Load, list, and update configurations in the Sensing API configuration database")

    parser.add_argument("mode", metavar="M", nargs='?', default="install", help="Top level interaction",
                        choices=["install", "list"])

    # what and how are we installing configurations
    parser.add_argument("-d", "--directory", dest="directory", default="./", help="Directory to recursively walk looking for configurations")
    parser.add_argument("-u", "--update", dest="update", default=False, action="store_true", help="Update already existing configurations")

    # where's the server?
    parser.add_argument("-a", "--api-host", dest="api_host", default="localhost", help="API host URI")
    parser.add_argument("--api-port-https", dest="api_port_https", default=17504, type=int, help="API HTTPS port")
    parser.add_argument("--api-port-http", dest="api_port_http", default=17141, type=int, help="API HTTP port")

    parser.add_argument("--api-version", dest="api_version", default="v1", help="API version being called")

    return parser.parse_args()


if __name__ == "__main__":

    print "Sensor Configuration Tool"

    opts = options()

    if opts.mode == "install":

        print "%% searching for configurations in [%s]" % (opts.directory,)
        configs = find_configuration_collections(opts.directory)

        for config in configs:
            install(opts, config)

    elif opts.mode == "list":

        list_action(opts)