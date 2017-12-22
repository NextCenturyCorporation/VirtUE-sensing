defmodule ApiServer.Plugs.Authenticate do
  import Plug.Conn
  import Phoenix.Controller

  def init(default), do: default
#
#  def call(%Plug.Conn{adapter: {Plug.Adapters.Cowboy.Conn, request}} = conn, _params) do
#    IO.puts("DECODING CLIENT CERTIFICATE")
#    socket = :cowboy_req.get(:socket, request)
#
#    case socket do
#      {:sslsocket, _, _} ->
#        {:ok, raw_certificate} = :ssl.peercert(socket)
#        {:"OTPCertificate", _, _, _} = :public_key.pkix_decode_cert(raw_certificate, :otp)
#        IO.puts("decoded raw certificate")
#        IO.puts("raw cert(\n#{raw_certificate}\n)")
##        name = PublicKeySubject.common_name()
#        conn
#      _ ->
#        IO.puts("couldn't decode an ssl socket")
#        conn
#    end
#  end

  def call(%Plug.Conn{private: %{client_certificate_common_name: tls_common_name}} = conn, _params) do
    IO.puts "user certificate CN(#{tls_common_name})"
    conn
  end

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