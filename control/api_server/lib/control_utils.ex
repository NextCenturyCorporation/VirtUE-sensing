defmodule ApiServer.ControlUtils do
  @moduledoc """
  Tools and methods for managing, advertising, and authenticating the control
  systems and metadata of the Sensing API.
  """

  @doc """
  Broadcast a message on the C2 channel about an observation change on a sensor.

  ### Parameters

    - sensor: ApiServer.Sensor instance
    - level: new observation level name

  ### Configuration

    - :api_server, :c2_kafka_topic - Kafka topic on which to announce Sensing API metadata

  ### Side Effects

    - JSON encoded message broadcast to Kafka C2 channel

  ### Returns

    {:ok, %Sensor{}}
  """
  def announce_sensor_observation(sensor, level) do
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(
      %{
        error: false,
        action: "sensor-observe",
        sensor: ApiServer.Sensor.clean_json(sensor, include_component: true, include_configuration: true),
        timestamp: DateTime.to_string(DateTime.utc_now()),
        old_level: sensor.configuration.level,
        new_level: level
      }
    )) do

      :ok ->
        IO.puts("announced sensor observe change sensor(id=#{sensor.sensor_id}) old level(#{sensor.configuration.level}) new level(#{level})")
      {:error, reason} ->
        IO.puts("got an error announcing a sensor observe change: #{reason}")
    end

    {:ok, sensor}
  end

  @doc """
  Broadcast an announcement of a new sensor.

  ### Parameters

    - sensor_struct_data: an ApiServer.Sensor instance
    - topic : Kafka channel created for the sensor

  ### Configuration

    - :api_server, :c2_kafka_topic - Kafka topic on which to announce Sensing API metadata

  ### Side effects

    - JSON encoded message broadcast to Kafka

  ### Returns

    {:ok, %Sensor{}}
  """
  def announce_new_sensor(sensor_struct_data) do

    produce_nonce_message(sensor_struct_data)

    announce_msg = %{
      error: false,
      action: "sensor-registration",
      sensor: ApiServer.Sensor.clean_json(sensor_struct_data, include_component: true, include_configuration: true),
      topic: sensor_struct_data.kafka_topic,
      timestamp: DateTime.to_string(DateTime.utc_now())
    }

    # Kafka C2
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(announce_msg)) do
      :ok ->
        IO.puts("announced new sensor(id=#{sensor_struct_data.sensor_id}) (topic=#{sensor_struct_data.kafka_topic})")
      {:error, reason} ->
        IO.puts("got some kinda error announcing a new sensor: #{reason}")
    end

    # WebSocket C2
    ApiServer.Endpoint.broadcast! "c2:all", "c2_msg", announce_msg

    {:ok, sensor_struct_data}
  end

  # Create a nonce style message against the topic we're announcing, or anyone who subscribes too early will
  # get booted from that topic, as it doesn't technically exist yet.
  defp produce_nonce_message(sensor_struct_data) do
    case KafkaEx.produce(sensor_struct_data.kafka_topic, 0, Poison.encode!(
      %{
        error: false,
        type: "nonce",
        topic: sensor_struct_data.kafka_topic,
        timestamp: DateTime.to_string(DateTime.utc_now())
      }
    )) do
      :ok ->
        IO.puts("published nonce message for sensor(id=#{sensor_struct_data.sensor_id}) (topic=#{sensor_struct_data.kafka_topic})")
      {:error, reason} ->
        IO.puts("got some kinda error publishing a new sensor nonce: #{reason}")
    end
  end

  @doc """
  Broadcast an announcement of a sensor being deregistered.

  ### Parameters

    - %Sensor{} struct - sensor being deregistered
    - topic - logging topic of the sensor

  ### Configuration

    - :api_server, :c2_kafka_topic - Kafka topic on which to announce Sensing API metadata

  ### Side Effects

    - JSON encoded message broadcast to Kafka

  ### Returns

    {:ok, %Sensor{}}
  """
  def announce_deregistered_sensor(sensor_struct_data) do

    announce_msg = %{
      error: false,
      action: "sensor-deregistration",
      sensor: ApiServer.Sensor.clean_json(sensor_struct_data, include_component: true, include_configuration: true),
      topic: sensor_struct_data.kafka_topic,
      timestamp: DateTime.to_string(DateTime.utc_now())
    }

    # Kafka C2
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(announce_msg)) do
      :ok ->
        IO.puts("announced sensor deregistration (topic=#{sensor_struct_data.kafka_topic})")
      {:error, reason} ->
        IO.puts("got some kinda error announcing a sensor deregistration: #{reason}")
    end

    # WebSocket C2
    ApiServer.Endpoint.broadcast! "c2:all", "c2_msg", announce_msg
  end

  @doc """
  Emit a heartbeat message on the Command and Control topic

  ### Side Effects

    - JSON encoded message broadcast to Kafka
  """
  def heartbeat() do

    heartbeat_msg = %{
      error: false,
      action: "heartbeat",
      timestamp: DateTime.to_string(DateTime.utc_now())
    }

    # C2 channel on Kafka
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(heartbeat_msg)) do
      :ok ->
        :ok
      {:error, reason} ->
        IO.puts("!! encountered an error broadcasting the C2 heartbeat to the API C2 topic")
    end

    # WebSocket C2
    ApiServer.Endpoint.broadcast! "c2:all", "c2_msg", heartbeat_msg

  end

  @doc """
  Emit a sensor status heartbeat. This includes counts of registered sensors and distinct registered
  hosts.
  """
  def sensor_status() do

    with host_count <- ApiServer.Sensor.host_count(),
      sensor_name_counts <- ApiServer.Sensor.sensor_name_count(),
      sensor_os_counts <- ApiServer.Sensor.sensor_os_count()
    do
      case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(
        %{
          error: false,
          action: "sensors-status",
          timestamp: DateTime.to_string(DateTime.utc_now()),
          hosts: host_count,
          sensor_type: sensor_name_counts,
          sensor_os: sensor_os_counts
        }
      )) do
        :ok ->
          :ok
        {:error, reason} ->
          IO.puts("!! encountered an error broadcasting a sensor count status heartbeat to the API C2 topic")
      end
    end

  end

end