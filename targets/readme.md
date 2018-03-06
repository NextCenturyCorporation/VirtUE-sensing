While developing our sensors, kernel modules, support tools, and infrastructure, we define our own **Virtues** as Docker containers composed of the various features under development. These containers can be added to running Sensing environments, and function just as VM based Virtues would within the environment, interacting with the Sensing API, Certificate Authority, and Logging systems.

Every **Target** is defined by a `target.json` file, like the [demo-target](https://github.com/twosixlabs/savior/blob/doc-rewrite-api-getting-started/targets/demo-target/target.json). Targets that are based off of the `demo-target` will easily integrate into the Sensing environment.

# Building Existing Targets

bin script

# Changing Sensors

altering the target.json

# Starting Targets

add target bin script (needs to be genericized)

# Creating a new Target

defined by target.json

## Defining a Target

basics

## Required Sensors

sensor list

## Required Kernel Modules

custom module list

## Dockerfile

basic layout and required sections

## Directory Layout

what goes where

## Required Scripts

how to copy demo-target scripts

## Tooling

dockerized-build.sh
add_taget.sh

# Adding Targets 

## To an Active Sensing Environment

add target script (how do we genericize this?)

clear network script

## To default Sensing Environment

modifying the docker-compose

# Development Workflow

1. change target
2. build target
3. add target
4. debug
5. clear network
6. rinse-repeat