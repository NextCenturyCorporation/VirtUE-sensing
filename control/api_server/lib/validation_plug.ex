defmodule ApiServer.ValidationPlug do
  import Phoenix.Controller
  import Plug.Conn

  # Validate that the OBSERVE level passed into an API endpoint is an acceptable value
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
  def valid_validate_action(%Plug.Conn{params: %{"action" => action}} = conn, _) do
    case action do
      action when action in ["canary", "cross-validation"] ->
        conn
      _ ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "invalid validate action[#{action}]"}
        Plug.Conn.halt(conn)
        conn
    end
  end

  def conn_invalid_parameter(conn) do
    conn
    |> put_status(400)
  end
end