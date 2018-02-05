defmodule ApiServer.Actuation do
  @moduledoc """
  Actuation routines for sensors and infrastructure components.
  """

  @doc """
  Actuate a sensor to a new observation level. The actuation routine
  will handle the lookup of the new configuration data.

  ### Actuation

  When the sensor's remote actuation HTTPS endpoint is called, it will
  receive an actuation bundle that looks like:

      {
          "timestamp": "...",
          "actuation": "observe",
          "level": "actuation level",
          "configuration": "string encoded configuration data"
      }

  When the sensor

  ### Return

    {:error, reason}
    {:error, http error}
    {:ok, %ApiServer.Sensor{}, level, configuration_data}
  """
  def sensor(%ApiServer.Sensor{}=sensor, "observe"=actuation, level) do

    # 1. get our configuration
    # 2. issue call to sensor
    # 3. determine our results
    with {:ok, c_sensor} <- ApiServer.Sensor.assign_configuration(sensor, %{level: level}, save: true),
         {:ok, configuration} <- ApiServer.Sensor.get_configuration_content(c_sensor),
         {:ok, %{status_code: 200}} <- put_sensor(c_sensor, "/actuation", actuation_payload(c_sensor, level, configuration))
    do

      {:ok, c_sensor, level, configuration}

    else
      :no_such_configuration ->
        IO.puts("Specified sensor does not have a configuration for level(#{level})")
        {:error, :no_such_configuration}

      {:error, :nxdomain} ->
        IO.puts("NXDOMAIN lookup error for sensor")
        {:error, :nxdomain}
      {:error, %{status_code: 500}} ->
        IO.puts("Sensor responded with a status_code(500)")
        {:error, "sensor https endpoint responded with a status_code(500) error"}

      {:error, reason} ->
        IO.puts("Could not alter or actuation observe level for sensor: #{reason}")
        {:error, reason}

    end

  end

  defp actuation_payload(%ApiServer.Sensor{} = sensor, level, data) do
    %{
      timestamp: DateTime.to_string(DateTime.utc_now()),
      actuation: "observe",
      level: level,
      payload: %{
        configuration: data,
        sensor: sensor.sensor_id,
        kafka_bootstrap_hosts: Application.get_env(:api_server, :sensor_kafka_bootstrap),
        sensor_topic: sensor.kafka_topic
      }
    }
  end

  defp put_sensor(%ApiServer.Sensor{}=sensor, path, %{} = payload) do

    actuation_url = "https://#{sensor.address}:#{sensor.port}#{path}"
    IO.puts("  = actuation uri(#{actuation_url})")
    case HTTPoison.put(actuation_url, Poison.encode!(payload), [], [ssl: [cacertfile: Application.get_env(:api_server, :ca_cert_file)], timeout: 5000, recv_timeout: 5000, connect_timeout: 5000]) do

      {:ok, %HTTPoison.Response{status_code: 200, body: _}} ->
        IO.puts("  - sensor actuated")
        {:ok, %{status_code: 200}}
      {:ok, %{status_code: 404}} ->
        IO.puts("sensor(#{sensor.id}) not responding on actuation path, status_code(404)")
        {:error, "Sensor doesn't support actuation"}
      {:ok, %HTTPoison.Response{status_code: 500}} ->
        IO.puts("  - error actuating sensor")
        {:error, %{status_code: 500}}
      {:error, %HTTPoison.Error{reason: reason}} ->
        IO.puts("  ! HTTPoison error: #{reason}")
        {:error, reason}
    end
  end
end