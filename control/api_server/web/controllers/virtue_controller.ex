
defmodule ApiServer.VirtueController do
  use ApiServer.Web, :controller


  def index(conn, _params) do
    json conn, %{virtues: []}
  end

  def show(conn, %{"id" => id}) do
    json conn_with_status(conn, :not_found), %{}
  end

  defp conn_with_status(conn, stat) do
    conn
      |> put_status(stat)
  end
end
