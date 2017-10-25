defmodule ApiServer.Plugs.Authenticate do
  import Plug.Conn
  import Phoenix.Controller

  def init(default), do: default

  def call(conn, _default) do
    case conn.method do

      # If this is a PUT request, make sure a userToken is in the PUT body
      "PUT" ->

        case Map.get(conn.body_params, "userToken") do

          nil ->
            json conn_not_authorized(conn), %{error: :true, msg: "user not authorized"}
            Plug.Conn.halt(conn)
            conn
          _ ->
            %{"userToken"=> t} = conn.body_params
            IO.puts "user token == #{t}"
            conn
        end

      # for GET requests we punt on authentication for now
      "GET" ->
        conn
    end
  end

  defp conn_not_authorized(conn) do
    conn
      |> put_status(401)
  end
end