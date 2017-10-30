defmodule ApiServer.TrustController do
  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_trust_action: 2]

  plug :valid_trust_action when action in [:trust]

  def trust(conn, _) do
    json conn_with_status(conn, 501), conn_json(conn, 501)
  end

  def trust(conn, _, _) do
    json conn_with_status(conn, 501), conn_json(conn, 501)
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
      msg: "Not Implemented - Handled by #{handled_by}",
      error: :true
    }
  end
end