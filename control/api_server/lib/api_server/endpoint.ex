defmodule ApiServer.Endpoint do
  use Phoenix.Endpoint, otp_app: :api_server

  socket "/socket", ApiServer.UserSocket

  # Serve at "/" the static files from "priv/static" directory.
  #
  # You should set gzip to true if you are running phoenix.digest
  # when deploying your static files in production.
  plug Plug.Static,
    at: "/", from: :api_server, gzip: false,
    only: ~w(css fonts images js favicon.ico robots.txt glyphicons-halflings-regular.ttf glyphicons-halflings-regular.woff glyphicons-halflings-regular.woff2)

  # Code reloading can be explicitly enabled under the
  # :code_reloader configuration of your endpoint.
  if code_reloading? do
    socket "/phoenix/live_reload/socket", Phoenix.LiveReloader.Socket
    plug Phoenix.LiveReloader
    plug Phoenix.CodeReloader
  end

  plug Plug.RequestId
  plug Plug.Logger

  plug Plug.Parsers,
    parsers: [:urlencoded, :multipart, :json],
    pass: ["*/*"],
    json_decoder: Poison

  plug Plug.MethodOverride
  plug Plug.Head

  # The session will be stored in the cookie and signed,
  # this means its contents can be read but not tampered with.
  # Set :encryption_salt if you would also like to encrypt it.
  plug Plug.Session,
    store: :cookie,
    key: "_api_server_key",
    signing_salt: "MX1UmHrI"

  plug ApiServer.Router
end
