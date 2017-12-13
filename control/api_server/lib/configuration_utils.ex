defmodule ApiServer.ConfigurationUtils do

  @moduledoc """
  Methods for finding and retrieving configuration data for sensors
  and infrastructure components.
  """

  @doc """
  Find the shared secret key for CFSSL.

  This checks, in order, the environment variable CFSSL_SHARED_SECRET and the configuration
  item config::api_server::cfssl_shared_secret. If neither of these is set, :no_shared_secret_available is returned.

  ### Returns

    - "shared secret"
    - :no_shared_secret_available

  """
  def get_cfssl_shared_secret() do

    case System.get_env("CFSSL_SHARED_SECRET") do
      nil ->
        case Application.get_env(:api_server, :c2_kafka_topic) do
          nil ->
            :no_shared_secret_available
          secret ->
            secret
        end
      secret ->
        secret
    end
  end

  @doc """
  Find the default configuration for a sensor by name lookup.

  In `match-prefix` mode, everything after the final `-` in the sensor name will be removed during
  lookup. This allows for sensor names to follow the pattern:

      sensor-name-can-be-long-versionnumber

  And still lookup sensor defaults with just the prefix `sensor-name-can-be-long`.

  ### Parameters

    - sensor_name : String name of the sensor (required)
    - match_prefix : Boolean flag - compare to default configurations based on prefix matching of sensor name (optional)

  ### Returns

    - {:ok, config}
    - {:error, reason}
  """
  def default_sensor_config_by_name(sensor_name, %{match_prefix: match_prefix}) do

    case lookup_default_sensor_config_by_name(sensor_name, %{match_prefix: match_prefix}) do
      {:ok, config} ->
        {:ok, Poison.encode!(config)}
      {:error, reason} ->
        {:error, reason}
    end

  end

  def default_sensor_config_by_name(sensor_name, _) do

    case lookup_default_sensor_config_by_name(sensor_name, %{match_prefix: false}) do
      {:ok, config} ->
        {:ok, Poison.encode!(config)}
      {:error, reason} ->
        {:error, reason}
    end

  end

  @doc """
  Check if a default configuration exists for the specified sensor.

  See `default_sensor_config/2` for details of the **options**

  ### Parameters

    - sensor_name : string name of the sensor (required)
    - match_prefix : Boolean flag - compare to default configuration based on prefix matching of sensor name (optional)

  ### Returns

    - :true
    - :false
  """
  def have_default_sensor_config_by_name(sensor_name, %{match_prefix: match_prefix}) do
    case lookup_default_sensor_config_by_name(sensor_name, %{match_prefix: match_prefix}) do
      {:ok, config} ->
        :true
      {:error, reason} ->
        :false
    end
  end

  def have_default_sensor_config_by_name(sensor_name, _) do
    case lookup_default_sensor_config_by_name(sensor_name, %{match_prefix: false}) do
      {:ok, config} ->
        :true
      {:error, reason} ->
        :false
    end
  end

  defp lookup_default_sensor_config_by_name(sensor_name, %{match_prefix: match_prefix}) do

    # create our lookup name based on prefix matching setting
    prefix = case match_prefix do
      :true ->
        sensor_name
          |> String.split("-")
          |> List.delete_at(-1)
          |> Enum.join("-")
      :false ->
        sensor_name
    end

    defaults = %{
      "lsof-sensor" => %{
        "repeat-interval" => 5
      }
    }

    # lookup our default configuration data
    case Map.has_key?(defaults, prefix) do

      :true ->
        {:ok, Map.get(defaults, prefix)}

      :false ->
        {:error, "Can't find default configuration for sensor(name=#{prefix})"}
    end

  end
end