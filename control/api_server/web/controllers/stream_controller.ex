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
  import ApiServer.ValidationPlug, only: [valid_log_level: 2, valid_follow: 2, valid_since: 2]

  plug :valid_log_level when action in [:stream]
  plug :valid_follow when action in [:stream]
  plug :valid_since when action in [:stream]
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

    # we'll try and build a Stream generator, which we can run
    # to spool out chunked data responses of JSONL as we receive
    # messages from Kafka
    case create_kafka_stream(send_chunked(conn, 200), stream_topic) do
      {:error, conn} ->
        IO.puts("Error retrieving Kafka topic for the targeted sensor")
        conn
      {cks_stream, conn} ->
        Stream.run(cks_stream)
        conn
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

        # get the MapSet we'll use when validating messages for pass through
        level_set = get_log_level_set(conn.assigns.filter_level)
        IO.puts("  ~ filtering out below log level [#{conn.assigns.filter_level}]")

        # we use the incoming since_date time, and the follow_log option when
        # we build the stream
        cks_stream = Stream.map(
          KafkaEx.stream(
            sensor, 0,
            offset: get_topic_offset(sensor, conn.assigns.since_datetime),
            no_wait_at_logend: !conn.assigns.follow_log
          ),
          fn (msg) ->

            # try and decode the incoming message. We've got NOP :ok's all
            # over the place in here, mostly because we want to just elide
            # messages that either don't decode or don't meet our filter level
            case Poison.decode(msg.value) do

              {:error, _} ->
                :ok
              {:ok, parsed_message} ->

                case Map.has_key?(parsed_message, "level") do
                  :true ->
                    IO.puts(parsed_message["level"])
                    case MapSet.member?(level_set, parsed_message["level"]) do

                      # this meets or exceeds our level filter
                      :true ->
                        chunk(conn, Poison.encode!(parsed_message) <> "\n")
                      :false ->
                        :ok
                    end
                  :false ->
                    :ok
                end
            end
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

  @doc """
  Retrieve the MapSet of log levels which are allowable message levels given
  a filtering level.
  """
  defp get_log_level_set(level) do
    Map.get(%{
      "everything": MapSet.new(["everything", "debug", "info", "warning", "error", "event"]),
      "debug": MapSet.new(["debug", "info", "warning", "error", "event"]),
      "info": MapSet.new(["info", "warning", "error", "event"]),
      "warning": MapSet.new(["warning", "error", "event"]),
      "error": MapSet.new(["error", "event"]),
      "event": MapSet.new(["event"])
    },
    level)
  end

  @doc """
  Get a topic offset given a topic and a date time.

  ### Returns

    - Intger >= 0
  """
  defp get_topic_offset(topic, dt) do
    offset_response =
    case KafkaEx.offset(topic, 0, NaiveDateTime.to_erl(dt)) do
      [%KafkaEx.Protocol.Offset.Response{partition_offsets: [%{offset: [num_offset]}]}| _] ->
        IO.puts("Found offset of #{num_offset} for #{dt} on topic #{topic}")
        num_offset
      [%KafkaEx.Protocol.Offset.Response{partition_offsets: [%{offset: []}]}| _] ->
        IO.puts("No offset available for #{dt} on topic #{topic}")
        0
      [] ->
        0
    end
  end

end