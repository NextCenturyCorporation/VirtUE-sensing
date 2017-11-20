defmodule ApiServer.ConfigurationUtils do

  @moduledoc """
  Methods for finding and retrieving configuration data for sensors
  and infrastructure components.
  """

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