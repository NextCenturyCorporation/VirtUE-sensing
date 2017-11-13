defmodule ApiServer.VersionController do
  use ApiServer.Web, :controller

  def version(conn, _) do
    conn
    |> put_status(200)
    |> json(
         %{
           error: :false,
           version: ApiServer.version(),
           timestamp: DateTime.to_string(DateTime.utc_now())
         }
       )
  end

end