#
# The ConfigureController provides methods for getting and setting the
# configuration of specific sensors in the SAVIOR architecture.
#
# Called as:
#
#     VERB "/path/to/:action", ApiServer.Configure, :action, name: "route-name"
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/10/30
#
defmodule ApiServer.ConfigureController do
  use ApiServer.Web, :controller
  import ApiServer.ExtractionPlug, only: [extract_sensor_id: 2]

  # get our Sensor ID into conn::sensor_id
  plug :extract_sensor_id when action in [:review, :configure]

  # Plug.Conn handler /2
  #
  # Retrieve the settings of a specific sensor, identified by
  # the sensor id :sensor in the path.
  #
  # Return:
  #   - HTTP 200 / Sensor config if valid sensor ID
  #   - HTTP 400 / Error message if invalid sensor ID
  #   - HTTP 504 / Timeout querying sensor
  #
  def review(conn, _) do
    IO.puts("ConfigureController -> Operating on sensor[#{conn.assigns.sensor_id}]")
    conn
      |> put_status(200)
      |> json(
        %{
          error: :false,
          sensor: conn.assigns.sensor_id,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          configuration: %{}
        }
       )
  end

  # Plug.Conn handler /2
  #
  # Update the settings of a specific sensor, identified by
  # the sensor id :sensor in the path. The JSON request body must
  # include:
  #
  #   - configuration: URI or base64 encoded configuration data
  #
  # Return:
  #   - HTTP 200 / Sensor config if valid sensor ID
  #   - HTTP 400 / Error message if invalid sensor ID
  #   - HTTP 400 / Missing configuration
  #   - HTTP 504 / Timeout querying and updating sensor
  #

  # configuration exists in request body
  def configure(%Plug.Conn{body_params: %{"configuration" => configuration}}=conn, _) do
    IO.puts("ConfigureController -> Operation on sensor [#{conn.assigns.sensor_id}]")

    # are we working with a URI to a config or a base64 encoding of a config
    cond do
      String.starts_with?(configuration, "http") ->
        # we've got a URI as config
        conn
         |> put_status(200)
         |> json(%{
              error: :false,
              sensor: conn.assigns.sensor_id,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              config_source: "uri",
              msg: "ok"
            })
      true ->
        # we've got base 64 as a config, which we need to try and decode to make
        # sense of
        case Base.decode64(configuration) do
          {:ok, _} ->
            conn
              |> put_status(200)
              |> json(
                  %{
                    error: :false,
                    sensor: conn.assigns.sensor_id,
                    timestamp: DateTime.to_string(DateTime.utc_now()),
                    config_source: "base64",
                    msg: "ok"
                  }
                 )
            :error ->
              conn
                |> put_status(400)
                |> json(
                    %{
                      error: :true,
                      msg: "Could not decode Base64 encoded configuration blob",
                      timestamp: DateTime.to_string(DateTime.utc_now())
                    }
                   )
        end
        conn
    end
  end

  # configuration does not exist in request body
  def configure(conn, _) do
    conn
      |> put_status(400)
      |> json(
          %{
            error: :true,
            msg: "Missing configuration payload",
            timestamp: DateTime.to_string(DateTime.utc_now())
          }
         )
  end

end