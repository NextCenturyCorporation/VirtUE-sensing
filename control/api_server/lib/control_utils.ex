defmodule ApiServer.ControlUtils do
  @moduledoc """
  Tools and methods for managing, advertising, and authenticating the control
  systems and metadata of the Sensing API.
  """

  @doc """
  Broadcast an announcement of a new sensor.

  ### Parameters

    - sensor_struct_data : a Sensor struct (see `sensor.ex`)
    - topic : Kafka channel created for the sensor

  ### Configuration

    - :api_server, :c2_kafka_topic - Kafka topic on which to announce Sensing API metadata

  ### Side effects

    - JSON encoded message broadcast to Kafka

  ### Returns

    {:ok, %Sensor{}}
  """
  def announce_new_sensor(sensor_stuct_data, topic) do

    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(
      %{
        error: false,
        action: "sensor-registration",
        sensor: sensor_stuct_data,
        topic: topic,
        timestamp: DateTime.to_string(DateTime.utc_now())
      }
    )) do
      :ok ->
        IO.puts("announced new sensor (topic=#{topic})")
      {:error, reason} ->
        IO.puts("got some kinda error announcing a new sensor: #{reason}")
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
  def announce_deregistered_sensor(sensor_struct_data, topic) do
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(
      %{
        error: false,
        action: "sensor-deregistration",
        sensor: sensor_struct_data,
        topic: topic,
        timestamp: DateTime.to_string(DateTime.utc_now())
      }
    )) do
      :ok ->
        IO.puts("announced sensor deregistration (topic=#{topic})")
      {:error, reason} ->
        IO.puts("got some kinda error announcing a sensor deregistration: #{reason}")
    end
  end

  @doc """
  Emit a heartbeat message on the Command and Control topic

  ### Side Effects

    - JSON encoded message broadcast to Kafka
  """
  def heartbeat() do
    case KafkaEx.produce(Application.get_env(:api_server, :c2_kafka_topic), 0, Poison.encode!(
      %{
        error: false,
        action: "heartbeat",
        timestamp: DateTime.to_string(DateTime.utc_now())
      }
    )) do
      :ok ->
        :ok
      {:error, reason} ->
        IO.puts("!! encountered an error broadcasting the C2 heartbeat to the API C2 topic")
    end
  end

end