#
# Routing action validation routines. These are executed inline to routing
# before the final handlers are called as Plug methods. Used inline with
# a Controller with:
#
#   import ApiServer.ValidationPlug, only: [valid_observe_level: 2]
#   :plug :valid_observe_level when action in [:observe]
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/10/30

defmodule ApiServer.ValidationPlug do
  import Phoenix.Controller
  import Plug.Conn

  # Validate that the OBSERVE level passed into an API endpoint is an acceptable value.
  #
  # Requires URI or QUERY parameters:
  #   - :level
  #
  # Response:
  #   - Continue if valid
  #   - Halt/HTTP 400 if invalid
  def valid_observe_level(%Plug.Conn{params: %{"level" => level}} = conn, _) do

    IO.puts "validating observation level #{level}"

    # simple check, halt and catch fire
    case level do
      level when level in ["off", "default", "low", "high", "adversarial"] ->
        conn
      _ ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "invalid observation level [#{level}]"}
        Plug.Conn.halt(conn)
        conn
    end
  end

  # Make sure the action of a TRUST command is valid
  #
  # Requires URI or QUERY parameters:
  #   - :action
  #
  # Response:
  #   - Continue if valid
  #   - Halt/HTTP 400 if invalid
  def valid_trust_action(%Plug.Conn{params: %{"action" => action}} = conn, _) do

    # simple check, as usual
    case action do
      action when action in ["validate", "invalidate"] ->
        conn
      _ ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "invalid trust action [#{action}]"}
        Plug.Conn.halt(conn)
        conn
    end
  end

  # Make sure the action of a validate command is valid
  #
  # Requires URI or QUERY parameters:
  #   - :action
  #
  # Response:
  #   - Continue if valid, putting action in conn::validate_action
  #   - Halt/HTTP 400 if invalid
  def valid_validate_action(%Plug.Conn{params: %{"action" => action}} = conn, _) do
    case action do
      action when action in ["canary", "cross-validation"] ->
        case action do
          "canary" ->
            conn
              |> assign(:validate_action, "canary-validate")
          "cross-validation" ->
            conn
              |> assign(:validate_action, "cross-validate")
        end

      _ ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "invalid validate action[#{action}]"}
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