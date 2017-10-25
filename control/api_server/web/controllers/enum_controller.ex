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

  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

end