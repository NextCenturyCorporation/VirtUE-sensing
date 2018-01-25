
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

  @deprecated "Use ApiServer.Sensor.update/3 instead"
  def update_pkikey(%PKIKey{:private_key_hash => private_key_hash} = pkikey) do
    case Mnesia.transaction(
      fn ->
        case Mnesia.match_object({PKIKey, :_, :_, :_, :_, :_, :_, :_, private_key_hash, :_}) do

          # no matches...
          [] ->
            :false

          # we've got a few matches...
          records ->

            Enum.each(records,
              fn (rec) ->
                pkikey
                  |> PKIKey.to_mnesia_record(%{:original => rec})
                  |> Mnesia.write()

              end
            )
            :true
        end
      end
     ) do
      {:atomic, :true} ->
        :ok
      {:atomic, :false} ->
        {:error, :no_such_record}
    end
  end

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



  @doc """
  Record a PKIKey structure for a new PKI request.

  Returns :mnesia transaction result

    {:ok}
    {:error, reason}

  """
  def record_private_key_request(%PKIKey{} = pkikey) do
    case Mnesia.transaction(
      fn ->
        Mnesia.write(PKIKey.to_mnesia_record(pkikey))
      end
     ) do
      {:atomic, :ok} ->
        {:ok, pkikey}
      {:atomic, {:aborted, reason}} ->
        {:error, "recording of new private key request aborted: #{reason}"}
    end
  end

  @doc """
  Retrieve the full PKIKey record for a given Certificate Signing request

  ### Parameters

    - csr: certificate signing request

  ### Returns

    {:ok, %PKIKey{}}
    {:error, reason}
  """
  def record_for_csr(csr) when csr != nil do

    case Mnesia.transaction(
           fn ->
             Mnesia.select(PKIKey, [{{PKIKey, :"$1", :"$2", :"$3", :"$4", :"$5", :"$6", :"$7", :"$8", :"$9"}, [{:==, :"$7", csr}], [:"$$"]}])
           end
         ) do

      {:atomic, []} ->
        {:error, :no_such_pkikey}
      {:atomic, [record|_]} ->
        {:ok, PKIKey.from_mnesia_record(record)}
    end

  end

  defp datetime_minutes_ago(mins) do

    Timex.now()
     |> Timex.subtract(%Timex.Duration{megaseconds: 0, microseconds: 0, seconds: 60 * mins})
     |> Timex.to_datetime()

  end

  # Given an :mnesia record, retrieve the address field
  #
  defp address_from_record(rec) do
    %{
      :sensor => elem(rec, index_for_key(:sensor_id)),
      :address => elem(rec, index_for_key(:address)),
      :port => elem(rec, index_for_key(:port))
    }
  end

  # Interpolate a key into our record position
  defp index_for_key(k) do
    Enum.find_index(
      [:id, :sensor_id, :virtue_id, :username, :address, :timestamp, :port, :public_key, :sensor_name, :kafka_topic],
      fn(x) ->
        x == k
      end
    )
  end

  # Make sure our value is an integer
  #
  defp as_intger(p) when is_integer(p) do
    p
  end

  defp as_intger(p) when is_bitstring(p) do
    elem(Integer.parse(p), 0)
  end
end