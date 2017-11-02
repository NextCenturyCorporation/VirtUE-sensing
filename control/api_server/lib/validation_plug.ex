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

    # simple check, halt and catch fire
    case level do
      level when level in ["off", "default", "low", "high", "adversarial"] ->
        conn
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid observation level [#{level}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
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
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid trust action [#{action}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Make sure the action of a validate command is valid
  #
  # Requires URI or QUERY parameters:
  #   - :action
  #
  # Response:
  #   - Continue if valid, putting action in conn::assigns:::validate_action
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
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid validate action [#{action}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Make sure the incoming log level parameter is valid
  #
  # Requires URI or QUERY parameters:
  #   - :filter_level
  #
  # Response:
  #   - Continue if valid, putting log level in conn:assigns::filter_level
  #   - Halt/HTTP 400 if invalid
  def valid_log_level(%Plug.Conn{params: %{"filter_level" => filter_level}} = conn, _) do
    case filter_level do
      filter_level when filter_level in ["everything", "debug", "info", "warning", "error", "event"] ->
        conn
          |> assign(:filter_level, filter_level)
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid log filter level [#{filter_level}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
    end
  end
end