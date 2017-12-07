defmodule ApiServer.StatsController do
  use ApiServer.Web, :controller

  alias :mnesia, as: Mnesia

  @doc """
  Determine if the API is ready to serve content.

  The structure returned by this endpoint will look like:

    {
      "ready": true | false,
      "timestamp": *timestamp*
    }

  ### Validations

    None

  ### Available Data

    None

  ### Return

    - HTTP 200 / JSON
  """
  def ready(conn, _) do

    case Mnesia.wait_for_tables([Sensor], 500) do
      :ok ->
        ready_response(conn, true)
      {:timeout, _} ->
        ready_response(conn, false)
      {:error, reason} ->
        ready_response(conn, false)
    end
  end

  defp ready_response(conn, ready) do
    conn
      |> put_status(200)
      |> json(
          %{
            "ready": ready,
            "timestamp": DateTime.to_string(DateTime.utc_now())
          }
         )
  end
end
