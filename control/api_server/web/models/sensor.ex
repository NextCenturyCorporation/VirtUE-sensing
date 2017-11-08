defmodule Sensor do
  @enforce_keys [:sensor]
  defstruct [:sensor, :virtue, :username, :address, :timestamp, :port]

  def sensor(sensor, virtue, username, address, timestamp, port) do
    %Sensor{sensor: sensor, virtue: virtue, username: username, address: address, timestamp: timestamp, port: port}
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

end