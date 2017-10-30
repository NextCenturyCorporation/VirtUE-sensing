defmodule ApiServer.StubController do
  use ApiServer.Web, :controller

  def not_implemented(conn) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  def not_implemented(conn, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  def not_implemented(conn, _, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  def not_implemented(conn, _, _, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

  defp conn_json(conn, code) do
    handled_by = controller_module(conn)
    %{
      path: current_path(conn),
      action: action_name(conn),
      status_code: code,
      msg: "Stub Route - Functionality unavailable - Handled by #{handled_by}",
      error: :true
    }
  end
end