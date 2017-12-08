
defmodule ApiServer.RegistrationController do
  @moduledoc """

  The RegistrationController provides methods for registering sensors
  with the API.

  Called as:

     VERB "/path/to/:action", ApiServer.RegisterController, :register, name: "route-name"

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/11/07
  """

  use ApiServer.Web, :controller
  import ApiServer.ExtractionPlug, only: [extract_sensor_id: 2, is_virtue_id: 1, is_sensor_id: 1, is_hostname: 1, is_username: 1, is_public_key: 1]
  alias :mnesia, as: Mnesia

  # get our Sensor ID into conn::sensor_id
  plug :extract_sensor_id when action in [:review, :configure]

  @doc """
  Deregister a sensor with the sensing API.

  This is a _Plug.Conn handler/2_.

  The sensor **must** be identified by sensor_id and public_key to
  successfully deregister.

  ### Validations:

  ### Available:

    - conn::assigns::sensor_id

  ### Return:

    - HTTP 200 / Sensor removed from tracking
    - HTTP 400 / Invalid or missing deregistration parameters in payload
    - HTTP 500 / Deregistration error
  """
  def deregister(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "sensor" => sensor,
            "public_key" => public_key_b64
          }
        } = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)

    IO.puts("Deregistering sensor(id=#{sensor})")

    cond do

      # make sure we have a valid sensor id
      ! is_sensor_id(sensor) ->
        invalid_deregistration(conn, "-", "sensor", sensor)
      ! is_public_key(public_key) ->
        invalid_deregistration(conn, sensor, "public_key", public_key)

      # now we have a valid deregistration to work with
      :true ->
        case ApiServer.DatabaseUtils.deregister_sensor(
          Sensor.with_public_key(Sensor.sensor(sensor), public_key)
        ) do
          {:error, :no_such_sensor} ->
            IO.puts("  error deregistering sensor(id=#{sensor}) - no such sensor ")
            conn
            |> put_status(400)
            |> json(
                %{
                  error: :true,
                  timestamp: DateTime.to_string(DateTime.utc_now()),
                  msg: "Error deregistering sensor: no such sensor registered"
                }
               )
          {:ok, sensor_structs} ->
            IO.puts("  #{length(sensor_structs)} sensor(s) deregistered")
            scount = Mnesia.table_info(Sensor, :size)
            IO.puts("  #{scount} sensors currently registered.")

            # run all of the structs through the deregistration announcement
            Enum.map(sensor_structs,
              fn (sensor_struct) ->
                ApiServer.ControlUtils.announce_deregistered_sensor(sensor_struct, sensor_struct.sensor)
              end
            )

            # spit out the results
            conn
            |> put_status(200)
            |> json(
                 %{
                   error: :false,
                   timestamp: DateTime.to_string(DateTime.utc_now()),
                   sensor: sensor,
                   deregistered: :true
                 }
               )
          {:error, reason} ->
            IO.puts("  error deregistering sensor: #{reason}")
            conn
            |> put_status(500)
            |> json(
                 %{
                   error: :true,
                   timestamp: DateTime.to_string(DateTime.utc_now()),
                   msg: "Error deregistering sensor: #{reason}"
                 }
               )
        end
    end
  end

  @doc """
  Sync a sensor and it's registration record, updating the registration time stamp.

  Sensors that fall too far out of sync will be automatically deregistered.

  This is a _Plug.Conn handler/2_.

  ### Validations

  ### Available

    - conn::assigns::sensor_id

  ### Return

    - HTTP 200 / Sensor synced
    - HTTP 400 / Invalid or missing sync parameters in payload
    - HTTP 410 / Sensor not currently registered or already deregistered
  """
  def sync(%Plug.Conn{method: "PUT", body_params: %{"sensor" => sensor, "public_key" => public_key_b64}} = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)

    # basic logging
    IO.puts("Syncing sensor(id=#{sensor})")

    # construct the sensor struct
    sensor_blob = Sensor.sensor(sensor) |> Sensor.with_public_key(public_key)

    cond do

      # we have a bunch of failure cases up front
      ! is_sensor_id(sensor) ->
        invalid_sync(conn, "-", "sensor", sensor)

      ! is_public_key(public_key) ->
        invalid_sync(conn, sensor, "public_key", public_key)

      ! ApiServer.DatabaseUtils.is_registered?(sensor_blob)
        invalid_sync(conn, sensor, "is registered", :false)

      # now we have a valid sync we can do
      :true ->

        # sync
        case ApiServer.DatabaseUtils.sync_sensor(sensor_blob) do

          :ok ->
            IO.puts("Synced sensor(id=#{sensor})")
            conn
              |> put_status(200)
              |> json(
                  %{
                    error: false,
                    timestamp: DateTime.to_string(DateTime.utc_now()),
                    sensor: sensor,
                    synchronized: true
                 }
                 )
          {:error, reason} ->
            IO.puts("Sync error sensor(id=#{sensor}): #{reason}")
            conn
              |> put_status(410)
              |> json(%{
                error: true,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                synchronized: false,
                msg: reason
            })
        end
    end
  end

  @doc """
  Register a sensor with the Sensing API

  This is a _Plug.Conn handler/2_.

  ### Validations:

  ### Available:

    - conn::assigns::sensor_id

  ### Return:

    - HTTP 200 / Sensor registered
    - HTTP 400 / Invalid or missing registration parameters in payload
    - HTTP 500 / Registration error
  """
  def register(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "sensor" => sensor,
            "virtue" => virtue,
            "user" => username,
            "public_key" => public_key_b64,
            "hostname" => hostname,
            "port" => port,
            "name" => sensor_name
          }
        } = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)

    # basic logging
    IO.puts("Registering sensor(id=#{sensor})")

    cond do

      # we have a bunch of failure cases up front
      ! is_sensor_id(sensor) ->
        invalid_registration(conn, "-", "sensor", sensor)
      ! is_virtue_id(virtue) ->
        invalid_registration(conn, sensor, "virtue", virtue)
      ! is_hostname(hostname) ->
        invalid_registration(conn, sensor, "hostname", hostname)
      ! is_username(username) ->
        invalid_registration(conn, sensor, "username", username)
      ! is_public_key(public_key) ->
        invalid_registration(conn, sensor, "public_key", public_key)
      ! Integer.parse(port) == :error ->
        IO.puts IEx.Info.info(port)
        invalid_registration(conn, sensor, "port", port)
      ! ApiServer.ConfigurationUtils.have_default_sensor_config_by_name(sensor_name, %{match_prefix: true}) ->
        invalid_registration(conn, sensor, "default configuration", "Cannot locate default configuration for #{sensor_name}")

      # now we have a valid registration
      :true ->

        # can we verify that this remote sensor is actually listening for actuation?
        case verify_remote_sensor(hostname, port, sensor) do

          # remote is listening, let's register it
          :ok ->
            case ApiServer.DatabaseUtils.register_sensor(
                   Sensor.sensor(sensor, virtue, username, hostname, DateTime.to_string(DateTime.utc_now()), port, public_key, sensor_name)
                   |> Sensor.randomize_kafka_topic()
                 ) do

              # sensor recorded, we're good to go
              {:ok, sensor_blob} ->

                # broadcast a control topic alert message
                ApiServer.ControlUtils.announce_new_sensor(sensor_blob)

                # record some registration metadata to the log
                IO.puts("  @ sensor(id=#{sensor}, name=#{sensor_name}) registered at #{DateTime.to_string(DateTime.utc_now())}")
                IO.puts("  <> sensor(id=#{sensor}) assigned kafka topic(id=#{sensor_blob.kafka_topic})")

                scount = Mnesia.table_info(Sensor, :size)
                IO.puts("  #{scount} sensors currently registered.")

                # send out the registration response
                conn
                |> put_status(200)
                |> json(
                     %{
                       error: :false,
                       timestamp: DateTime.to_string(DateTime.utc_now()),
                       sensor: sensor,
                       registered: :true,
                       kafka_bootstrap_hosts: Application.get_env(:api_server, :sensor_kafka_bootstrap),
                       sensor_topic: sensor_blob.kafka_topic,
                       default_configuration: elem(ApiServer.ConfigurationUtils.default_sensor_config_by_name(sensor_name, %{match_prefix: true}), 1)
                     }
                   )

              # couldn't record the sensor for some reason, this is a problem
              {:error, reason} ->
                IO.puts("  error registering sensor: #{reason}")
                conn
                |> put_status(500)
                |> json(
                     %{
                       error: :true,
                       timestamp: DateTime.to_string(DateTime.utc_now()),
                       msg: "Error registering sensor: #{reason}"
                     }
                   )
            end

          # remote isn't responding, this is a problem
          :error ->

            conn
              |> put_status(417) # timeout: expectation failed
              |> json(
                   %{
                    error: :true,
                    timestamp: DateTime.to_string(DateTime.utc_now()),
                    msg: "Could not contact the sensor with a verification ping, registration incomplete"
                   }
                 )

        end

    end
  end

  def register(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "One or more missing fields in the registration payload"
         }
       )
  end

  @doc """
  Send a verification ping to a remote sensor.

  During the regstration process, sensors are verified via side channel round trip HTTP
  calls. If the sensor returns an HTTP/200, we can complete the registration. The sensor
  ID is sent as part of the HTTP request, which the sensor should verify as being it's
  correct sensor ID.

  ### Params:

    - **hostname**: bare hostname of sensor, without any leading scheme
    - **port**: integer port monitored by sensor
    - **sensor**: ID of the sensor being verified

  ### Returns:

    - :ok - sensor verified
    - :error - verification failed
  """
  def verify_remote_sensor(hostname, port, sensor) do

    # TODO: this request will happen over HTTPS, and we'll need to pass our custom
    # root CA to the HTTPoison hackney instance, with something like:
    #
    #   HTTPoison.get("https://example.com/", [], ssl: [cacertfile: "/app/certs/ca.pem"])
    #
    # see: https://github.com/edgurgel/httpoison/issues/294

    # let's send out our verification ping
    case HTTPoison.get("http://#{hostname}:#{port}/sensor/#{sensor}/registered", [], [timeout: 5000, recv_timeout: 5000, connect_timeout: 5000]) do

      {:ok, %HTTPoison.Response{status_code: 200, body: _}} ->
        IO.puts("  + sensor(id=#{sensor}) verified with direct ping")
        :ok
      {:ok, %HTTPoison.Response{status_code: 401}} ->
        IO.puts("  - sensor(id=#{sensor}) verification mismatch from direct ping")
        :error
      {:ok, %HTTPoison.Response{status_code: 500}} ->
        IO.puts("  - internal server error from sensor(id=#{sensor}) during verification ping")
        :error
      {:ok, %HTTPoison.Response{status_code: status_code}} ->
        IO.puts("  - status_code == #{status_code} from sensor(id=#{sensor}) during verification ping")
        :error
      {:error, %HTTPoison.Error{reason: reason}} ->
        IO.puts("  ! HTTPoison error during sensor(id=#{sensor}) verification ping: #{reason}")
        :error
    end
  end

  defp invalid_sync(conn, sensor_id, key, value) do
    IO.puts("! sensor(id=#{sensor_id}) failed synchronization with (#{key} = #{value})")
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "sensor(id=#{sensor_id}) failed synchronization with (#{key} = #{value})"
         }
       )
    |> Plug.Conn.halt()
  end

  # Build the HTTP/400 JSON response to an invalid registration attempt. This call
  # will halt the connection.
  defp invalid_registration(conn, sensor_id, fieldname, value) do
    IO.puts("! sensor(id=#{sensor_id}) failed registration with invalid field (#{fieldname} = #{value})")
    conn
    |> put_status(400)
    |> json(
         %{
          error: :true,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          msg: "sensor(id=#{sensor_id}) failed registration with invalid field (#{fieldname} = #{value})"
         }
       )
    |> Plug.Conn.halt()
  end

  # Build the HTTP/400 JSON response to an invalid deregistration request. This call will
  # halt the connection.
  defp invalid_deregistration(conn, sensor_id, fieldname, value) do
    IO.puts("! sensor(id=#{sensor_id}) failed deregistration with invalid field (#{fieldname} = #{value})")
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "sensor(id=#{sensor_id}) failed deregistration with invalid field (#{fieldname} = #{value})"
         }
       )
    |> Plug.Conn.halt()
  end

end