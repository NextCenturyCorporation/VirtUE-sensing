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
    json conn_with_status(conn, 200), \
      %{
        error: :false,
        sensor: conn.assigns.sensor_id,
        timestamp: DateTime.to_string(DateTime.utc_now()),
        configuration: %{}
      }
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
  def configure(conn, _) do
    IO.puts("ConfigureController -> Operating on sensor[#{conn.assigns.sensor_id}]")
    json conn_with_status(conn, 501), conn_json(conn, 501)
  end

  # Private Method
  #
  # Apply an HTTP Status to the connection and return
  # the connection for chaining.
  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

  # Private Method / Temporary Method
  #
  # Return the JSON blob used during testing/development, which
  # will be sent on to the connection.
  defp conn_json(conn, code) do
    handled_by = controller_module(conn)
    %{
      path: current_path(conn),
      action: action_name(conn),
      status_code: code,
      msg: "Not Implemented - Handled by #{handled_by}",
      error: :true
    }
  end
end