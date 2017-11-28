defmodule ApiServer.StreamController do
  @moduledoc """
  Retrieve the log stream from a set of active sensors.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """
  use ApiServer.Web, :controller

  import Supervisor.Spec
  import UUID, only: [uuid4: 0]

  import ApiServer.ExtractionPlug, only: [extract_targeting: 2, extract_targeting_scope: 2]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]
  import ApiServer.ValidationPlug, only: [valid_log_level: 2, valid_follow: 2, valid_since: 2]

  plug :valid_log_level when action in [:stream]
  plug :valid_follow when action in [:stream]
  plug :valid_since when action in [:stream]
  plug :extract_targeting when action in [:stream]
  plug :extract_targeting_scope when action in [:stream]

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
    - HTTP 500 / JSONL: stream or targeting error
  """
  def stream(conn, _) do

    # log targeting data
    ApiServer.TargetingUtils.log_targeting(conn.assigns.targeting, conn.assigns.targeting_scope)


    # Let's see if this sensor really exists
    case ApiServer.TargetingUtils.select_sensors_from_targeting(conn.assigns.targeting, conn.assigns.targeting_scope) do

      # no sensors selected by targeting
      {:ok, []} ->
        IO.puts("  0 sensors found for streaming")
        conn
          |> put_status(500)
          |> json(
              Map.merge(%{
                error: true,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                reason: "No sensors found matching targeting",
                },
                summarize_targeting(conn.assigns)
              )
             )

      # we've got one or more sensor, let's start all of the sterams
      {:ok, sensor_structs} ->

        stream_topics = Enum.map(sensor_structs, fn (s) -> s.kafka_topic end)
        stream_remote_ip = remote_ip_to_string(conn.remote_ip)

        # Basic logging so we know what's going on
        IO.puts("  #{length(sensor_structs)} sensors found for streaming")
        IO.puts("  <> attempting to stream from #{length sensor_structs} sensors to #{stream_remote_ip}")

        # spin up all of the streams, this will return a Plug.Conn when the stream is broken
        spawn_stream(send_chunked(conn, 200), stream_topics)

      # error finding this sensor, we're gonna bail
      {:error, reason} ->
        IO.puts("  ! Attempting to stream from a sensor caused an error [#{reason}]")
        conn
          |> put_status(500)
          |> json(
              %{
                error: true,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                msg: reason
              }
             )
    end
  end

  @doc """
  Setup tasks to monitor each topic stream and a gather task that collates
  the messages from the streaming tasks into a single output stream to the
  client connection.

  This method will finish when the client stream is broken

  ### Parameters

    - %Plug.Conn - Client connection already set for chunked response
    - [string, ...] - list of one or more kafka topics

  ### Side effects

  This method uses `Task.async` to spawn and manage stream and collation tasks,
  which are cleaned up with `Task.shutdown` when the client connection is broken.

  ### Returns

    - %Plug.Conn
  """
  def spawn_stream(conn, topics) do

    IO.puts("  <> Starting stream with #{length topics} topics")

    # launch the gather
    gather_task = Task.async(__MODULE__, :stream_gather, [self(), conn])

    # launch the streaming tasks
    stream_tasks = Enum.map(topics, fn (topic) ->
      Task.async(__MODULE__, :stream_task, [gather_task.pid, conn, topic])
    end)

    # wait
    receive do
      :stream_gather_done ->
        IO.puts("  <!> Streaming complete.")
    end

    # shut it all down
    IO.puts("  <🛑> Stopping streams and gather tasks")
    Task.shutdown(gather_task)
    Enum.each(stream_tasks, fn (t) -> Task.shutdown(t) end)
    conn
  end

  @doc """
  Gather messages from the streaming tasks, encoded them, and send them
  to the client.

  ### Parameters

    - Parent PID
    - %Plug.Conn - Client connection already set to chunked response

  ### Messaging

  If a client connection error is detected during streaming, recursion
  is halted and a `:stream_gather_done` message is sent to the task parent.

  ### Returns

    - n/a
  """
  def stream_gather(parent, conn) do

    receive do

      # we've got a decode JSON message from one of our topics
      {:ok, msg} ->
        case chunk(conn, Poison.encode!(msg) <> "\n") do
          {:ok, _} ->
            stream_gather(parent, conn)

          {:error, term} ->

            # client has likely disconnected, let's kill this stream
            send(parent, :stream_gather_done)
        end

      # one of our topics doesn't exist
      {:error, :no_such_topic, topic} ->
        IO.puts("  ! topic(id=#{topic}) doesn't exist")
        chunk(conn, Poison.encode!(%{error: true, message: "No such topic"}) <> "\n")
        stream_gather(parent, conn)
    end
  end

  @doc """
  Establish a streaming connection to Kafka for a topic, forwardig received messages
  to the Gather task.

  This method must be stopped by the Task parent, or the message stream must be exhausted
  and timed out or broken.

  The Task parent should kill this task with `Task.shutdown`.

  ### Parameters

    - parent - Gather task PID
    - %Plug.Conn - Client connection
    - topic - String, kafka topic

  ### Messaging

    - {:error, :no_such_topic, topic} - If topic doesn't yet exist in kafka
    - {:ok, %{}} - Parsed message payload from Kafka

  ### Returns

    - n/a
  """
  def stream_task(parent, conn, topic) do

    # make sure the topic exists
    case KafkaEx.latest_offset(topic, 0) do

      # doesn't exist - we send an error JSON
      :topic_not_found ->
        send(parent, {:error, :no_such_topic, topic})
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
            topic, 0,
            offset: get_topic_offset(topic, conn.assigns.since_datetime),
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
                    case MapSet.member?(level_set, parsed_message["level"]) do

                      # this meets or exceeds our level filter
                      :true ->
                        send(parent, {:ok, parsed_message})
                      :false ->
                        :ok
                    end
                  :false ->
                    :ok
                end
            end
          end
        )
        |> Stream.run()
    end
  end

  # Quick formatter for turning a connections remote IP into a loggable string
  defp remote_ip_to_string({a, b, c, d}) do
    "#{a}.#{b}.#{c}.#{d}"
  end

  defp remote_ip_to_string(_) do
    "unknown"
  end

  @doc """
  Retrieve the MapSet of log levels which are allowable message levels given
  a filtering level.
  """
  def get_log_level_set(level) do
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
        IO.puts("  <=> Found offset of #{num_offset} for #{dt} on topic #{topic}")
        num_offset
      [%KafkaEx.Protocol.Offset.Response{partition_offsets: [%{offset: []}]}| _] ->
        IO.puts("  <=> No offset available for #{dt} on topic #{topic}")
        0
      [] ->
        0
    end
  end

end