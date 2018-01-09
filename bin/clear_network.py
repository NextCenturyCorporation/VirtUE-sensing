#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
Clear out any non-savior containers from the savior network.
"""

import json
import subprocess


def find_containers():
    """
    Enumerate containers connected to the savior_default network
    that aren't prefixed with 'savior': these are the containers
    that need to be detached.

    :return: List of strings
    """

    network_raw = subprocess.check_output("docker network inspect savior_default", shell=True)

    network = json.loads(network_raw)

    containers = []
    for id, config in network[0]["Containers"].items():
        if not config["Name"].startswith("savior_"):
            containers.append(config["Name"])

    return containers


def stop_container(name):
    """
    Stop a docker container.

    :param name: Container name
    :return:
    """

    msg = "  🛑 stopping [%s]".decode("utf-8") % (name,)
    print msg
    res = subprocess.check_output("docker stop %s" % (name,), shell=True)
    print "     %s" % (res,)


if __name__ == "__main__":
    print "Looking for containers to remove from [savior_default]"
    containers = find_containers()
    print "  = found %d containers to remove" % (len(containers),)

    for container in containers:
        stop_container(container)