defmodule Sensor do
  @enforce_keys [:sensor]
  defstruct [:sensor, :virtue, :username, :address, :timestamp, :port, :public_key, :sensor_name]

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

  def with_virtue(sensor_struct, virtue) do
    %Sensor{sensor_struct | virtue: virtue}
  end

  def with_username(sensor_struct, username) do
    %Sensor{sensor_struct | username: username}
  end

  def with_address(sensor_struct, address) do
    %Sensor{sensor_struct | address: address}
  end

  def with_address(sensor_struct, address, port) do
    %Sensor{sensor_struct | address: address, port: port}
  end

  def with_public_key(sensor_struct, public_key) do
    %Sensor{sensor_struct | public_key: public_key}
  end

  def named(sensor_struct, sensor_name) do
    %Sensor{sensor_struct | sensor_name: sensor_name}
  end
end