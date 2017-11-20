
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

# Run Time

The Sensing API run time is comprised of a set of services that run concurrently to
provide the entire API capability.

## API Services

The API routing and response are handled by the [Elixir Phoenix](http://phoenixframework.org) 
framework. These routes provide standard REST capabilities. A few routes provide or use extra
functionality: 
 
### Sensor Registration

Sensor registration is handled via an API Service route as well as a side channel HTTP request
back to the registering sensor. The registration cycle is a key component. The Sensing API is
using the [HTTPoison](https://hexdocs.pm/httpoison/HTTPoison.html) library for generating HTTP
requests.

### Log Streaming

TBD 

## Data Persistence

Internally the Sensing API uses the distributed, resilient [Mnesia](http://erlang.org/doc/man/mnesia.html)
data store for persisting sensor registration, routing, and related data. See the [Data Models](#data-models)
section below for more.

## Schedule Tasks

Asynchronous task scheduling for the Sensing API is done via the [Quantum](https://hexdocs.pm/quantum/readme.html)
library. The tasks are defined in [config.exs](config/config.exs) as `cron` like tasks in the **Schedule** section.

The following tasks are currently configured:

### Sensor Pruning

Sensor Pruning removes sensor registration data for any sensor that hasn't sent a heartbeat in the
last 15 minutes. This is to cleanup after any sensors that were unable to deregister properly.

### Sensing Heartbeat

It's nice to know the Sensing API is still alive. [Elixir](https://pragtob.wordpress.com/2017/07/26/choosing-elixir-for-the-code-not-the-performance/), [Erlang OTP](https://en.wikipedia.org/wiki/Open_Telecom_Platform), 
[Phoenix](http://phoenixframework.org/blog/the-road-to-2-million-websocket-connections) and the [BEAM](https://pragprog.com/articles/erlang) are
ridiculously robust, so sometimes it's nice to get a "sign of life" out of the system, otherwise it can go for
_a really long time_ without emitting a log message.

The Sensing API heartbeat happens once a minute. 

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
  - **lib/database_utils.ex** `ApiServer.DatabaseUtils.prune_old_sensors/0` - remove old sensors from tracking
  - **web/controllers/registration_controller.ex** - `ApiServer.RegistrationController.register` - add relevant fields to `register` route
  