defmodule Sensor do

  import UUID, only: [uuid4: 0]

  @enforce_keys [:sensor]
  defstruct [:sensor, :virtue, :username, :address, :timestamp, :port, :public_key, :sensor_name, :kafka_topic]

  @doc """
  Create a new sensor struct.

  ### Parameters

    - sensor, virtue, username, address, timestamp, port, public_key, sensor_name, kafka_topic
    - sensor, virtue, username, address, timestamp, port, public_key, sensor_name
    - sensor, virtue, username, address, timestamp, port, public_key
    - sensor, virtue, timestamp
    - sensor

  ### Returns

    - %Sensor{}
  """
  def sensor(sensor, virtue, username, address, timestamp, port, public_key, sensor_name, kafka_topic) do
    %Sensor{
      sensor: sensor,
      virtue: virtue,
      username: username,
      address: address,
      timestamp: timestamp,
      port: port,
      public_key: public_key,
      sensor_name: sensor_name,
      kafka_topic: kafka_topic
    }
  end

  def sensor(sensor, virtue, username, address, timestamp, port, public_key, sensor_name) do
    %Sensor{
      sensor: sensor,
      virtue: virtue,
      username: username,
      address: address,
      timestamp: timestamp,
      port: port,
      public_key: public_key,
      sensor_name: sensor_name
    }
  end

  def sensor(sensor, virtue, username, address, timestamp, port, public_key) do
    %Sensor{sensor: sensor, virtue: virtue, username: username, address: address, timestamp: timestamp, port: port, public_key: public_key}
  end

  def sensor(sensor, virtue, timestamp) do
    %Sensor{sensor: sensor, virtue: virtue, username: nil, address: nil, timestamp: timestamp, port: nil}
  end

  def sensor(sensor) do
    %Sensor{sensor: sensor, virtue: nil, username: nil, address: nil, timestamp: DateTime.to_string(DateTime.utc_now()), port: nil}
  end

  @doc """
  Update the Virtue ID being monitored by a sensor.

  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - virtue / unique ID of a virtue

  ### Returns

   - %Sensor{}
  """
  def with_virtue(sensor_struct, virtue) do
    %Sensor{sensor_struct | virtue: virtue}
  end

  @doc """
  Update the username being monitored by the sensor


  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - username / string

  ### Returns

   - %Sensor{}
  """
  def with_username(sensor_struct, username) do
    %Sensor{sensor_struct | username: username}
  end

  @doc """
  Update the address of a sensor without updating the port.


  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - address / string

  ### Returns

   - %Sensor{}
  """
  def with_address(sensor_struct, address) do
    %Sensor{sensor_struct | address: address}
  end

  @doc """
  Update the address and port where a sensor is listening for instructions.

  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - address / string
    - port / integer

  ### Returns

   - %Sensor{}
  """
  def with_address(sensor_struct, address, port) do
    %Sensor{sensor_struct | address: address, port: port}
  end

  @doc """
  Update the public key assigned to a sensor.


  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - public_key - decoded public key PEM data

  ### Returns

   - %Sensor{}
  """
  def with_public_key(sensor_struct, public_key) do
    %Sensor{sensor_struct | public_key: public_key}
  end

  @doc """
  Update the name of a sensor.


  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - sensor_name / string

  ### Returns

   - %Sensor{}
  """
  def named(sensor_struct, sensor_name) do
    %Sensor{sensor_struct | sensor_name: sensor_name}
  end

  @doc """
  Update the timestamp of the sensor to the current UTC date/time.

  This is a chainable method.

  ### Parameters

    - %Sensor{}

  ### Returns

   - %Sensor{}
  """
  def touch_timestamp(sensor_struct) do
    %Sensor{sensor_struct | timestamp: DateTime.to_string(DateTime.utc_now())}
  end

  @doc """
  Generate a randomized Kafka stream topic for use with this sensor.

  It only makes sense to call this method when originally generating the Sensor
  record that will be persisted to Mnesia and returned to a Sensor as part
  of the registration process.

  ### Parameters

    - %Sensor{}

  ### Returns

    - %Sensor{}

  """
  def randomize_kafka_topic(sensor_struct) do
    %Sensor{sensor_struct | kafka_topic: uuid4()}
  end

  @doc """
  Convert an Mnesia record into a %Sensor{} struct.

  ### Parameters

    - Mnesia tuple {Sensor, id, ...}
    - Mnesia list [id, ...]

  ### Returns

    - %Sensor{}

  """
  def from_mnesia_record(rec) when is_tuple(rec) do
    args = rec |> Tuple.to_list() |> tl |> tl
    apply(Sensor, :sensor, args)
  end

  def from_mnesia_record(rec) when is_list(rec) do
    args = rec |> tl
    apply(Sensor, :sensor, args)
  end

  @doc """
  Strip out field values that may pose a security risk if leaked outside of the Sensing API
  boundary.

  ### Parameters

    - %Sensor{}

  ### Returns

    - %Sensor{}

  """
  def sanitize(sensor_struct) do
    %Sensor{sensor_struct | kafka_topic: nil, port: nil}
  end

  @doc """
  Convert a Sensor struct to our Mnesia record format for sensors

  ### Parameters

    - %Sensor{}

  ### Returns

    - mnesia Sensor record compatible tuple
  """
  def to_mnesia_record(
        %Sensor{
          sensor: sensor,
          virtue: virtue,
          username: username,
          address: address,
          timestamp: timestamp,
          port: port,
          public_key: public_key,
          sensor_name: sensor_name,
          kafka_topic: kafka_topic
        },
        %{original: original_record}
      ) do
    mnesia_id = elem(original_record, 1)
    List.to_tuple([Sensor, mnesia_id, sensor, virtue, username, address, timestamp, port, public_key, sensor_name, kafka_topic])
  end
end