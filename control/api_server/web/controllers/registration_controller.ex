#
# The RegistrationController provides methods for registering sensors
# with the API.
#
# Called as:
#
#     VERB "/path/to/:action", ApiServer.RegisterController, :register, name: "route-name"
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/11/07
#
defmodule ApiServer.RegistrationController do
  use ApiServer.Web, :controller
  import ApiServer.ExtractionPlug, only: [extract_sensor_id: 2, is_virtue_id: 1, is_sensor_id: 1, is_hostname: 1, is_username: 1, is_public_key: 1]
  alias :mnesia, as: Mnesia

  # get our Sensor ID into conn::sensor_id
  plug :extract_sensor_id when action in [:review, :configure]

  @doc """
  Plug.Conn handler /2

  Deregister a sensor with the sensing API.

  The sensor **must** be identified by sensor_id and public_key to
  successfully deregister.

  Return:
    - HTTP 200 / Sensor removed from tracking
    - HTTP 400 / Invalid or missing deregistration parameters in payload
    - HTTP 500 / Deregistration error
  """
  def deregister(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "sensor" => sensor,
            "public_key" => public_key
          }
        } = conn, _) do

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
          {:ok, number_deregistered} ->
            IO.puts("  #{number_deregistered} sensor(s) deregistered")
            scount = Mnesia.table_info(Sensor, :size)
            IO.puts("  #{scount} sensors currently registered.")
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
   Plug.Conn handler /2

   Register a sensor with the Sensing API

   Return:
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
            "public_key" => public_key,
            "hostname" => hostname,
            "port" => port
          }
        } = conn, _) do

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

      # now we have a valid registration
      :true ->

        # can we verify that this remote sensor is actually listening for actuation?
        case verify_remote_sensor(hostname, port, sensor) do

          # remote is listening, let's register it
          :ok ->
            case ApiServer.DatabaseUtils.register_sensor(
                   Sensor.sensor(sensor, virtue, username, hostname, DateTime.to_string(DateTime.utc_now()), port, public_key)
                 ) do

              # sensor recorded, we're good to go
              {:ok} ->

                # send out the registration response
                IO.puts("  sensor registered")
                scount = Mnesia.table_info(Sensor, :size)
                IO.puts("  #{scount} sensors currently registered.")
                conn
                |> put_status(200)
                |> json(
                     %{
                       error: :false,
                       timestamp: DateTime.to_string(DateTime.utc_now()),
                       sensor: sensor,
                       registered: :true
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

  def verify_remote_sensor(hostname, port, sensor) do
    # let's send out our verification ping
    case HTTPoison.get("http://#{hostname}:#{port}/sensor/#{sensor}/registered") do

      {:ok, %HTTPoison.Response{status_code: 200, body: body}} ->
        IO.puts("  + sensor verified with direct ping")
        :ok
      {:ok, %HTTPoison.Response{status_code: 401}} ->
        IO.puts("  - verification mismatch from direct ping")
        :error
      {:ok, %HTTPoison.Response{status_code: 500}} ->
        IO.puts("  - internal server error from sensor during verification ping")
        :error
      {:ok, %HTTPoison.Response{status_code: status_code}} ->
        IO.puts("  - status_code == #{status_code} from sensor during verification ping")
        :error
      {:error, %HTTPoison.Error{reason: reason}} ->
        IO.puts("  ! HTTPoison error during verification ping: #{reason}")
        :error
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