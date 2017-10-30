#
# Plugs for extracting parameters from incoming request paths
# for insertion into the active connection.
#
# Called as:
#
#   import ApiServer.ParameterPlug, only: [extract_sensor_id: 2]
#   :plug :extract_sensor_id when action in [:observe]
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/10/30
defmodule ApiServer.ExtractionPlug do
  import Phoenix.Controller
  import Plug.Conn

  # Extract a Sensor ID labeled `:sensor` from the incoming
  # request path.
  #
  # Requires URI or QUERY parameters:
  #   - :sensor
  #
  # Response:
  #   - Continue if valid, assign Sensor ID to conn::sensor_id
  #   - Halt/HTTP 400 if invalid
  def extract_sensor_id(%Plug.Conn{params: %{"sensor" => sensor}} = conn, _) do

    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      String.match?(sensor, ~r/[a-zA-Z0-9]{8}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{12}/) ->
        assign(conn, :sensor_id, sensor)
      true ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "Invalid or missing Sensor ID"}
        Plug.Conn.halt(conn)
        conn
    end
  end

  # Simple wrapper method for adding an HTTP return code to a connection, and
  # returning the connection chain.
  def conn_invalid_parameter(conn) do
    conn
    |> put_status(400)
  end

end