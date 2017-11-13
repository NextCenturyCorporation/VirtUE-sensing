defmodule ApiServer.EnumController do
  @moduledoc """
  Simple static accessors for common settings and terms in the API.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """
  use ApiServer.Web, :controller

  @doc """
  The set of observational stances that can be applied to a set of sensors
  observing a target.

  This is a _Plug.Conn handler/2_.

  ### Returns:

    - HTTP 200 / JSON document
  """
  def observation_levels(conn, _) do
    conn
      |> put_status(200)
      |> json(
          %{
            error: :false,
            levels: ["off", "default", "low", "high", "adversarial"],
            timestamp: DateTime.to_string(DateTime.utc_now())
          }
         )
  end

  @doc """
  The set of log levels that can be used to filter log streams from
  a targeted set of observed virtues/sensors.

  This is a _Plug.Conn handler/2_.

  ### Returns:

    - HTTP 200 / JSON document
  """
  def log_levels(conn, _) do
    conn
      |> put_status(200)
      |> json(
            %{
              error: false,
              log_levels: ["everything", "debug", "info", "warning", "error", "event"],
              timestamp: DateTime.to_string(DateTime.utc_now())
            }
         )
  end
end