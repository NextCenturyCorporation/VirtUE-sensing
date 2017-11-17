defmodule ApiServer.StreamController do
  @moduledoc """
  Retrieve the log stream from a set of active sensors.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """
  use ApiServer.Web, :controller

  import UUID, only: [uuid4: 0]

  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]
  import ApiServer.ValidationPlug, only: [valid_log_level: 2]

  plug :valid_log_level when action in [:stream]
  plug :extract_targeting when action in [:stream]

  @doc """
  Find, filter, and stream back to the client a set of log messages
  from one or more sensors defined by the incoming targeting.

  This is a _Plug.Conn handler/2_.

  This method may or may not automatically terminate, dependinf on
  the _follow_ flag set by the requester.

  ### Validations:

    - `valid_log_level` - value in log level term set

  ### Available data:

    - conn::assigns::targeting - key/value propery map of target selectors

  ### Returns:

    - HTTP 200 / JSONL: newline delimited json stream
  """
  def stream(conn, _) do

    stream_topic = conn.assigns.targeting.sensor
    IO.puts(" <> attempting to stream from (topic=#{stream_topic})")

    case create_kafka_stream(send_chunked(conn, 200), stream_topic) do
      {:error, conn} ->
        IO.puts("Error retrieving Kafka topic for the targeted sensor")
        conn
      {cks_stream, conn} ->
        Stream.run(cks_stream)
    end

  end

  # temporary data generation
  defp create_message_stream(conn) do
    Enum.each(1..:rand.uniform(20),
        fn(_) ->
          {:ok, _} = chunk(conn, Poison.encode!(create_message(conn)) <> "\n")
        end
    )

    conn
  end

  # Connect to Kafka and stream the messages from the given channel
  defp create_kafka_stream(conn, sensor) do

    # make sure the topic exists
    case KafkaEx.latest_offset(sensor, 0) do

      # doesn't exist - we send an error JSON
      :topic_not_found ->
        chunk(conn, Poison.encode!(%{error: true, message: "No such topic"}) <> "\n")
        {:error, conn}

      # looks like it exists, let's start streaming
      _ ->
        cks_stream = Stream.map(KafkaEx.stream(sensor, 0, offset: 0),
          fn (msg) ->
            chunk(conn, Poison.encode!(%{message: msg.value}) <> "\n")
          end
        )
        {cks_stream, conn}
    end
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