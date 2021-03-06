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

      case kakfa_ready?() do
        :true ->
          ready_response(conn, true)
        :false ->
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

  defp kakfa_ready?() do

    try do
      case KafkaEx.latest_offset("asdf", 0) do
        :topic_not_found ->
          :true
        [%KafkaEx.Protocol.Offset.Response{}] ->
          :true
        _ ->
          :false
      end
    rescue
      _ ->
        :false
    end
  end
end
