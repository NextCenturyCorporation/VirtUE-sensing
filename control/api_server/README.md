
A high performance API server for control, orchestration, streaming, and actuation of
sensors in the SAVIOR system.

The Sensing API is written in Elixir, and runs on the Erlang BEAM VM. The API is designed
for scalable, reliable, and fault tolerant operation in a highly concurrent environment.

# Installation

## Elixir and Erlang

Follow the [Elixir Installation instructions](https://elixir-lang.org/install.html) to configure
your environment with at least **Elixir** version >= 1.52, and **Erlang** version >= 18.0. 

## Dependencies

Install the dependencies for the Sensing API:

  * Install dependencies with `mix deps.get`
  * Create and migrate your database with `mix ecto.create && mix ecto.migrate`
  

# Running

If you've followed the quick installation guide, and have already installed the server
dependencies with `mix deps.get`, you can start the server with:

```bash
mix phoenix.server
```  

Now you can visit [`localhost:4000/version`](http://localhost:4000/version) from your browser to
verify you have the server running.

# Development

## Version bumping

The version string of the Sensing API is in the `api_server.ex` module in the `lib` directory. It
follows a compacted date string format of `YYYYMMDD`.

## Routing

All of the HTTP routes supported by the Sensing API are defined in the `router.ex` module in
the `web` directory, and conform to the Sensing API Documentation.

## Controllers

Functionality for the API is broken into controllers, with each controller handling a **Sensing Action**,
like *observe* or *stream*. Each controller responds to the different **Observation Contexts** (like *Virtue* or *User*) that
support each action.

## Targeting

The Sensing API supports direct addressing of sensors, as well as **targeting**, depending on the observational
context. *Targeting* involves one or more selectors, each of which narrows down the set of sensors being
addressed. All of the following are valid *targeting*:

 - select a *sensor* by **sensor id**, identifying a single sensor
 - select a *user* by **username**, identifying all sensors observing the user
 - select a *virtue* by **virtue id** and an *application* by **application id**, identifying all sensors observing the given application running within the given virtue
 
More about *targeting* can be found in the Sensing API document.

## Data Models

The API Server maintains relevant sensor registration data in a disc persisted, distributed
[Mnesia](http://erlang.org/doc/man/mnesia.html) [datastore](https://elixirschool.com/en/lessons/specifics/mnesia/#nodes). The schema for the data store is defined in the `start_mnesia/0` method of the
**lib/api_server.ex** _ApiServer_ module.

Updates to the schema of the data table are handled in the same `start_mnesia/0` method, and
follow a process of updating from known previous forms to the current schema.

For the **Sensor** data model, a helper _struct_ is defined in the **web/models/sensor.ex** _Sensor_
module.

Utilities for working with the data model, _Sensor_ struct, and the Mnesia data store are in
the **lib/database_utils.ex** _ApiServer.DatabaseUtils_ module.

When updating the schema for any data tables, modifications must be made in the following
locations:

  - **lib/api_server.ex** `ApiServer.start_mnesia/0` in create_table - append new fields to end of list
  - **lib/api_server.ex** `ApiServer.start_mnesia/0` in each table_info/2 clause - alter updates from old data schemas to new data schema
  - **web/models/sensor.ex** `Sensor.defstruct` - append new fields to end of struct list
  - **web/models/sensor.ex** `Sensor.sensor/*` - extend appropriate methods with the new fields
  - **lib/database_utils.ex** `ApiServer.DatabaseUtils.register_sensor/1` - include new fields from `Sensor` struct in parameter matching for method call, and record creation with `Mnesia.write/1`
  - **lib/database_utils.ex** `ApiServer.DatabaseUtils.index_for_key/1` - append new fields to end of lookup list
 