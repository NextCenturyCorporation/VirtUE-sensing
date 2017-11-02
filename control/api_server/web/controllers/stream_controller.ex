defmodule ApiServer.StreamController do
  use ApiServer.Web, :controller

  import UUID, only: [uuid4: 0]

  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]
  import ApiServer.ValidationPlug, only: [valid_log_level: 2]

  plug :valid_log_level when action in [:stream]
  plug :extract_targeting when action in [:stream]

  def stream(conn, _) do
    conn
      |> send_chunked(200)
      |> create_message_stream()
  end

  defp create_message_stream(conn) do
    Enum.each(1..:rand.uniform(20),
        fn(x) ->
          {:ok, conn} = chunk(conn, Poison.encode!(create_message(conn)) <> "\n")
        end
    )

    conn
  end

  defp create_message(conn) do
    Map.merge(
      %{
        error: :false,
        timestamp: DateTime.to_string(DateTime.utc_now()),
        log_level: Enum.random(["everything", "debug", "info", "warning", "error", "event"]),
        sensor: uuid4(),
        message: "this is a log message with a number #{:rand.uniform(1000)}"
      },
      summarize_targeting(conn.assigns)
    )
  end

end