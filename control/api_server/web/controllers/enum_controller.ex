defmodule ApiServer.EnumController do
  use ApiServer.Web, :controller

  def observation_levels(conn, _) do
    json conn_with_status(conn, 200), %{
      error: :false,
      levels: [
        "off", "default", "low", "high", "adversarial"
      ]
    }
  end

  def log_levels(conn, _) do
    json conn_with_status(conn, 200), %{
      error: :false,
      log_levels: [
        "everything", "debug", "info", "warning", "error", "event"
      ]
    }
  end

  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

end