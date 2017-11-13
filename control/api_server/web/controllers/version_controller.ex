defmodule ApiServer.VersionController do
  @moduledoc """
  Versioning and non-authenticated system status.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """

  use ApiServer.Web, :controller

  @doc """
  Retrieve the version of the ApiServer code base.

  This is a _Plug.Conn handler/2_.

  ### Validations:

  ### Available:

  ### Returns:

    - HTTP 200 / JSON document of Sensing API version
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