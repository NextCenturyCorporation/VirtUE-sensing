defmodule ApiServer.Plugs.Authenticate do
  import Plug.Conn
  import Phoenix.Controller

  def init(default), do: default

  # put request with userToken auth
  def call(%Plug.Conn{:method => "PUT", body_params: %{"userToken" => token}} = conn, _default) do
      IO.puts "user token == #{token}"
      conn
  end

  # put request without userToken auth
  def call(%Plug.Conn{:method => "PUT"} = conn, _default) do
      conn
        |> conn_not_authorized()
        |> Plug.Conn.halt()
  end

  # Get request with userToken param
  def call(%Plug.Conn{:method => "GET", :params => %{"userToken" => token}} = conn, _params) do
    IO.puts "user token == #{token}"
    conn
  end

  # Get request without userToken param
  def call(%Plug.Conn{:method => "GET"} = conn, _params) do
      conn
        |> conn_not_authorized()
        |> Plug.Conn.halt()
  end


  defp conn_not_authorized(conn) do
    conn
      |> put_status(401)
      |> json(
          %{
            error: :true,
            msg: "user not authorized",
            timestamp: DateTime.to_string(DateTime.utc_now())
          }
         )
  end
end