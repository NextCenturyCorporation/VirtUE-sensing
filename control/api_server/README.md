
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