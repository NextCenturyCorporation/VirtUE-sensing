defmodule ApiServer.VersionController do
  @moduledoc """
  Versioning and non-authenticated system status.
  """
  use ApiServer.Web, :controller

  @doc """
  Retrieve the version of the ApiServer code base.
  """
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