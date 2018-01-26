
defmodule ApiServer.DatabaseUtils do
  @moduledoc """

  General utilities and helpers for working with the
  storage, query, and retrieval of sensors metadata.

  The database being used is defined in `web/models/database.ex`,
  and is a disk backed :mnesia database.
  """
  import UUID, only: [uuid4: 0]
  alias :mnesia, as: Mnesia
  use Timex

  @doc """
  Remove sensors that have exceeded the synchronization time limit.

  ### Parameters

    - prune_age : age, in minutes, to use as the synchronization limit. Sensors with timestamps older will be pruned

  ### Side Effects

    - sensors removed from internal :mnesia tracking
    - announcments on kafka command and control topic

  ### Returns

    nil

  """
  @deprecated "Functionality moving to ApiServer.Sensor methods"
  def prune_old_sensors(prune_age) do

    IO.puts("DatabaseUtils::prune_old_sensors/0 - #{DateTime.to_string(DateTime.utc_now())}")
    IO.puts("  = #{ApiServer.Sensor.count()} sensor registrations")
    IO.puts("  = pruning sensors with checkins older than #{prune_age} minutes")


    # Grab all of our records
    case ApiServer.Sensor.sensors_older_than(prune_age) do
      [] ->
        IO.puts("  * no sensor checkins older than #{prune_age} minutes")

      sensors ->
        IO.puts("  * #{length(sensors)} sensor checkins older than #{prune_age} minutes")
        Enum.map(sensors,
          fn (sensor) ->

            # prune out the old sensor
            ApiServer.Repo.delete(sensor)

            # various c2 and log announcements
            IO.puts("  - pruning sensor_id(#{sensor.sensor_id}})")
            ApiServer.ControlUtils.announce_deregistered_sensor(sensor)

          end
        )

        IO.puts("  - #{length(sensors)} sensor entries removed")

    end
  end

end