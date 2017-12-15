
defmodule ApiServer.DatabaseUtils do
  @moduledoc """

  General utilities and helpers for working with the
  storage, query, and retrieval of sensors metadata.

  The database being used is defined in `web/models/database.ex`,
  and is a disk backed :mnesia database.

  A utility Struct for Sensor objects is defined in:
  `web/models/sensor.ex`.
  """
  import UUID, only: [uuid4: 0]
  alias :mnesia, as: Mnesia
  use Timex

  @doc """
  Update the timestamp in the internal tracker for the given sensor, identified by it's sensor ID and
  public key.


  ### Parameters

    - %Sensor{} data struct

  ### Side Effects

    - Internal tracking database updates

  ### Returns

    :ok
    {:error, reason}

  """
  def sync_sensor(%Sensor{:sensor => sensor, :public_key => public_key}) do
    case Mnesia.transaction(
           fn ->
             case Mnesia.match_object(
                    {
                      Sensor,
                      :_,
                      sensor,
                      :_, :_, :_, :_, :_,
                      public_key, :_, :_
                    }
                  ) do

               # no matches
               [] ->
                 :false

               # records to remove
               records ->

                 Enum.map(records,
                 fn (rec) ->

                   rec
                    |> Sensor.from_mnesia_record()
                    |> Sensor.touch_timestamp()
                    |> Sensor.to_mnesia_record(%{:original => rec})
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
        {:error, :no_such_sensor}
    end

  end

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
  def prune_old_sensors(prune_age) do

    IO.puts("DatabaseUtils::prune_old_sensors/0 - #{DateTime.to_string(DateTime.utc_now())}")
    IO.puts("  = #{Mnesia.table_info(Sensor, :size)} sensor registrations")
    IO.puts("  = pruning sensors with checkins older than #{prune_age} minutes")

    # determine our record key
    case Mnesia.transaction(
      fn ->
        case Mnesia.select(Sensor,
          [
            {
              # match
              {
                Sensor,
                :"$1", :"$2", :"$3", :"$4", :"$5", :"$6", :"$7", :"$8", :"$9", :"$10"
              },

              # guard (timestamp older than 15 minutes
              [
                {
                  :<,
                  :"$6",
                  DateTime.to_string(datetime_minutes_ago(prune_age))
                }
              ],

              # result (full record)
              [
                :"$$"
              ]
            }
          ]
        ) do

          # nuthin'
          [] ->
            IO.puts("  * no sensor checkins older than #{prune_age} minutes")
            {:ok, 0}
          # we've got some old records! Let's prune 'em out!
          records ->
            IO.puts("  * #{length(records)} sensor checkins older than #{prune_age} minutes")
            Enum.map(records,
              fn (rec) ->

                # prune out the old sensor
                rec_t = List.to_tuple([Sensor] ++ rec)
                IO.puts("  - pruning sensor(id=#{elem(rec_t, 2)})")
                Mnesia.delete_object(rec_t)

                # announce the pruning on the C2 channel
                Sensor.from_mnesia_record(rec_t)
                  |> ApiServer.ControlUtils.announce_deregistered_sensor(elem(rec_t, index_for_key(:sensor_id) + 1))
              end
            )
            {:ok, length(records)}
        end
      end
    ) do
      {:atomic, {:ok, remove_count}} ->
        IO.puts("  - #{remove_count} sensor entries removed")
    end
  end

  @doc """
  Test whether a sensor identified by sensor ID and public key is already
  registered.

  Sensors are matched based on exact matching of both the **sensor ID**
  and the **public key**. Call the function as:

    DatabaseUtils.is_registered?(%Sensor{sensor: "sensor id", public_key: "public key"})

  Or with the Sensor helper as:

    Sensor.sensor("sensor_id")
      |> Sensor.with_public_key("public key")
      |> DatabaseUtils.is_registered?()

  ### Returns:

    :true
    :false
  """
  def is_registered?(%Sensor{:sensor => sensor, :public_key => public_key}) do
    case Mnesia.transaction(
           fn ->
             case Mnesia.match_object(
                    {
                      Sensor,
                      :_,
                      sensor,
                      :_, :_, :_, :_, :_,
                      public_key, :_, :_
                    }
                  ) do

               # no matches
               [] ->
                 :false

               # records to remove
               records ->
                 :true
             end
           end
         ) do
      {:atomic, :true} ->
        :true
      {:atomic, :false} ->
        :false
    end
  end

  @doc """
  Retrieve the full Sensor record for a sensor id.

  ### Parameters

    - sensor - %Sensor{} struct with sensor id set

  ### Returns:

    {:ok, record}
    {:error, reason}
  """
  def record_for_sensor(%Sensor{:sensor => sensor}) when sensor != nil do
    case Mnesia.transaction(
           fn ->
             case Mnesia.match_object(
                    {
                      Sensor,
                      :_,
                      sensor,
                      :_, :_, :_, :_, :_,
                      :_, :_, :_
                    }
                  ) do

               # no matches
               [] ->
                 {:error, :no_such_sensor}

               # one or more matching records. we return the first of them
               records ->
                 {:ok, Sensor.from_mnesia_record(List.first(records))}
             end
           end
         ) do
      {:atomic, {:ok, sensor_struct}} ->
        {:ok, sensor_struct}
      {:atomic, {:error, :no_such_sensor}} ->
        {:error, :no_such_sensor}
      {:atomic, {:aborted, reason}} ->
        {:error, "deregistration aborted: #{reason}"}
    end
  end

  @doc """
  Retrieve the full sensor records for a given Virtue

  ### Parameters

    - virtue - Virtue ID

  ### Returns

    {:ok, [%Sensor{},...]}
    {:error, reason}
  """
  def record_for_virtue(virtue) when virtue != nil do

    case Mnesia.transaction(
      fn ->
        Mnesia.select(Sensor, [{{Sensor, :"$1", :"$2", :"$3", :"$4", :"$5", :"$6", :"$7", :"$8", :"$9", :"$10"}, [{:==, :"$3", virtue}], [:"$$"]}])
      end
    ) do

      {:atomic, []} ->
        {:error, :no_such_sensor}
      {:atomic, records} ->
        {:ok, Enum.map(records, fn(rec) -> Sensor.from_mnesia_record(rec) end)}
    end

  end

  @doc """
  Remove a sensor from the tracking database.

  Sensors are matched based on exact matching of both the **sensor ID**
  and the **public key**. Call the function as:

    DatabaseUtils.deregister_sensor(%Sensor{sensor: "sensor id", public_key: "public key"})

  Or with the Sensor helper as:

    Sensor.sensor("sensor_id")
      |> Sensor.with_public_key("public key")
      |> DatabaseUtils.deregister_sensor()

  ### Returns:

    {:ok, number_of_removed_records}
    {:error, reason}
  """
  def deregister_sensor(
        %Sensor{
          :sensor => sensor,
          :public_key => public_key
        } = sensor_blob
      )
    do

    # remove any records matching this sensor ID, if we can
    # validate that the sensor ID and public key match
    case Mnesia.transaction(
      fn ->
        case Mnesia.match_object(
          {
            Sensor,
            :_,
            sensor,
            :_, :_, :_, :_, :_,
            public_key, :_, :_
          }
        ) do

          # no matches
          [] ->
            IO.puts("  : no matches when trying to deregister sensor(id=#{sensor})")
            IO.puts("    public key =")
            IO.puts(public_key)

            {:error, :no_such_sensor}

          # records to remove
          records ->

            sensor_structs = Enum.map(records,
                fn (rec) ->
                  Mnesia.delete_object(rec)
                  Sensor.from_mnesia_record(rec)
                end
            )
            {:ok, sensor_structs}
        end
      end
    ) do
      {:atomic, {:ok, remove_count}} ->
        {:ok, remove_count}
      {:atomic, {:error, :no_such_sensor}} ->
        {:error, :no_such_sensor}
      {:atomic, {:aborted, reason}} ->
        {:error, "deregistration aborted: #{reason}"}
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

  @doc """
  Register sensors to the database. Fields that aren't
  yet defined will be autogenerated or nil'ed by the
  registration method.

  Returns :mnesia transaction result

    {:ok}
    {:error, reason}
  """
  def register_sensor(
        %Sensor{
          :sensor => sensor,
          :virtue => virtue,
          :username => username,
          :address => address,
          :timestamp => nil,
          :port => port,
          :public_key => public_key,
          :sensor_name => sensor_name,
          :kafka_topic => kafka_topic
        } = sensor_blob)
    do

    # simple insert, create the timestamp on the fly
    case Mnesia.transaction(
      fn ->
        Mnesia.write(
          {
            Sensor,
            uuid4(),
            sensor,
            virtue,
            username,
            address,
            DateTime.to_string(DateTime.utc_now()),
            as_intger(port),
            public_key,
            sensor_name,
            kafka_topic
          }
        )
      end
    ) do
      {:atomic, :ok} ->
        {:ok, sensor_blob}
      {:atomic, {:aborted, reason}} ->
        {:error, "registration aborted: #{reason}"}
    end
  end

  def register_sensor(
        %Sensor{
          :sensor => sensor,
          :virtue => virtue,
          :username => username,
          :address => address,
          :timestamp => timestamp,
          :port => port,
          :public_key => public_key,
          :sensor_name => sensor_name,
          :kafka_topic => kafka_topic
        } = sensor_blob)
    do

    # simple insert
    case Mnesia.transaction(
      fn ->
        Mnesia.write(
          {
            Sensor,
            uuid4(),
            sensor,
            virtue,
            username,
            address,
            timestamp,
            as_intger(port),
            public_key,
            sensor_name,
            kafka_topic
          }
        )
      end
    ) do
      {:atomic, :ok} ->
        {:ok, sensor_blob}
      {:atomic, {:aborted, reason}} ->
        {:error, "registration aborted: #{reason}"}
    end
  end



  @doc """
  Retrieve the sensor address (hostname and port) using a
  sensor ID.

  Returns:

    {:ok, {:sensor => sensor_id, :addresss => hostname, :port => host port}
    {:error, reason}
  """
  def address_for_sensor(%{:sensor => sensor}) do

    Mnesia.transaction(
      fn ->
        case Mnesia.index_read(Sensor, sensor, :sensor) do
          {:atomic, []} ->
            {:error, "no sensor registered with id=#{sensor}"}
          {:atomic, payload} ->
            {:ok, address_for_sensor(List.first(payload))}
        end
      end
    )
  end

  @doc """
  Retrieve the list of sensors and addresses using any
  of the non :sensor_id indexed fields.

  Indexed fields include:

    - virtue_id
    - username
    - address

  Returns:

    {:ok, [{:sensor => "id", :address => "hostname", :port => "port"}, ...]}
    {:error, reason}
  """
  def addresses_for_sensor(%{:virtue => virtue}) do
    addresses_for_key(:virtue_id, virtue)
  end

  def addresses_for_sensor(%{:username => username}) do
    addresses_for_key(:username, username)
  end

  def addresses_for_sensor(%{:address => address}) do
    addresses_for_key(:address, address)
  end

  # Given a key (like :virtue_id) and a value (like a UUID), retrieve
  # all of the sensor addresses that match the k:v query in the
  # database.
  defp addresses_for_key(k, v) do
    Mnesia.transaction(
      fn ->
        case Mnesia.index_read(Sensor, v, k) do
          {:atomic, []} ->
            {:error, "no sensors found for #{k}=#{v}"}
          {:atomic, payload} ->
            {
              :ok,
              Enum.map(payload,
                fn (rec) ->
                  address_from_record(rec)
                end
              )
            }
        end
      end
    )
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