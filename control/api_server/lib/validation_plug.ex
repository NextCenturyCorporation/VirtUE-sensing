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

  def valid_log_level(%Plug.Conn{params: %{"level" => level}} = conn, _) do
    case level do
      level when level in ["everything", "debug", "info", "warning", "error", "event"] ->
        conn
      _ ->
        json conn_invalid_parameter(conn), %{error: :true, msg: "invalid log level [#{level}]"}
        Plug.Conn.halt(conn)
        conn
    end
  end

  def conn_invalid_parameter(conn) do
    conn
    |> put_status(400)
  end
end