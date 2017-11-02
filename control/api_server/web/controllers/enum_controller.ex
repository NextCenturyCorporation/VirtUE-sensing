defmodule ApiServer.EnumController do
  use ApiServer.Web, :controller

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