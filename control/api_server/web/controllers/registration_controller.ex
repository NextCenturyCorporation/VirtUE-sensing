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

  # Plug.Conn handler /2
  #
  # Register a sensor with the Sensing API
  #
  # Return:
  #   - HTTP 200 / Sensor registered
  #   - HTTP 400 / Invalid or missing registration parameters in payload
  #   - HTTP 500 / Registration error
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
        case ApiServer.DatabaseUtils.register_sensor(
          Sensor.sensor(sensor, virtue, username, hostname, DateTime.to_string(DateTime.utc_now()), port)
        ) do
          {:ok} ->
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

end