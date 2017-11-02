#
# The StubController is basically a NOP as far as routing and response are concerned,
# and is generally useful as a first stage stand in for new routing that isn't yet
# handled by a dedicated controller, but is already in routing.
#
# Called as:
#
#     VERB "/path/to/:action", ApiServer.StubController, :not_implemented, name: "route-name"
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/10/23
#
defmodule ApiServer.StubController do
  use ApiServer.Web, :controller

  # Plug.Conn handler /1 - simple JSON return
  def not_implemented(conn) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  # Plug.Conn handler /2 - simple JSON return
  def not_implemented(conn, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  # Plug.Conn handler /3 - simple JSON return
  def not_implemented(conn, _, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  # Plug.Conn handler /4 - simple JSON return
  def not_implemented(conn, _, _, _) do
    json conn_with_status(conn, 500), conn_json(conn, 500)
  end

  # Simple wrapper for applying a status code to a connection
  # and returning the connection chain
  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

  # All of the :not_implemented methods return the same JSON,
  # so let's make it concise and share it
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