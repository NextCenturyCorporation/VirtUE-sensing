defmodule ApiServer.TargetingUtils do
  alias :mnesia, as: Mnesia

  # We need to nicely format our targeting data
  def summarize_targeting(target_key_map) do
    target_key_map
  end

  @doc """
  Create a log message about what is being targeted in a target selector
  compatible query.
  """
  def log_targeting(targeting, targeting_scope) do

    IO.puts("🎯 summary")
    IO.puts("   scope: #{targeting_scope}")
    Enum.map(targeting, fn{k, v} ->
        IO.puts("    + #{k} == #{v}")
      end
    )
  end

  def log_targeting() do
    IO.puts("🎯 summary")
    IO.puts("   targeting ALL sensors")
  end

  @doc """
  Use targeting metadata to select a list of sensors.

  ### Parameters

    - targeting : Targeting data as extracted by ApiServer.ExtractionPlug.extract_targeting/2
    - query scope: Scope of the targeting data as extracted by ApiServer.ExtractionPlug.extract_scope/2

  ### Returns

    {:ok, [%Sensor{},...]}

  """
  def select_sensors_from_targeting(raw_targeting, targeting_scope) do

    targeting =
      raw_targeting
      |> rename_key(:virtue, :virtue_id)
      |> rename_key(:sensor, :sensor_id)


    case ApiServer.Sensor.get_many(targeting) do
      :no_matches ->
        IO.puts("  * no sensors matching targeting")
        {:ok, []}
      records ->
        IO.puts("  * #{length(records)} sensor(s) matching targeting")
        {:ok, records}
    end
  end

  defp rename_key(map, old_key, new_key) do
    case Map.has_key?(map, old_key) do
      true ->
        v = Map.get(map, old_key)
        map
        |> Map.delete(old_key)
        |> Map.put(new_key, v)
      false ->
        map
    end
  end

end