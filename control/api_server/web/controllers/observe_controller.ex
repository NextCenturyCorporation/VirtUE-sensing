defmodule ApiServer.ObserveController do
  @moduledoc """
  Set or retrieve the observation level for a set of sensors.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """

  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_observe_level: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2, extract_targeting_scope: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_observe_level when action in [:observe]
  plug :extract_targeting when action in [:observe]
  plug :extract_targeting_scope when action in [:observe]

  @doc """
  Set the observation level of the sensors within the targeting
  spec.

  This is a _Plug.Conn handler/2_.

  This may alter the level or runtime of one or more sensors,
  including halting or starting sensors.

  ### Validations:

    - `valid_observe_level` - value is in observation level set

  ### Available:

    - conn::assigns::targeting - key/value propery map of target selectors

  ### Returns:

    - HTTP 200 / JSON document with observation results
  """
  def observe(%Plug.Conn{params: %{"level" => level}} = conn, _) do

    # logging
    ApiServer.TargetingUtils.log_targeting(conn.assigns.targeting, conn.assigns.targeting_scope)

    # let's see if our targeting matches any active sensors
    case ApiServer.TargetingUtils.select_sensors_from_targeting(conn.assigns.targeting, conn.assigns.targeting_scope) do

      # no sensors selected
      {:ok, []} ->
        IO.puts("  0 sensors selected for observation actuation")
        conn |> respond_success(level, [])

      {:ok, sensors} ->
        IO.puts("  #{length(sensors)} sensor(s) selected for observation actuation")

        actuated_sensors = Enum.map(sensors, fn (sensor) ->
          actuate(sensor, "observe", level)
        end)

        conn |> respond_success(level, actuated_sensors)

      {:error, reason} ->
        IO.puts("  targeting selection caused an error")
        conn
        |> put_status(200)
        |> json(
            %{
              error: true,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              msg: reason
            }
           )
    end

  end

  # handle an actuation
  defp actuate(%ApiServer.Sensor{} = sensor, action, level) do

    case ApiServer.Actuation.sensor(sensor, action, level) do
      {:ok, c_sensor, c_level, config} ->
        {:ok, c_sensor}
      {:error, :nxdomain} ->
        IO.puts("Sensor(#{sensor.id}) hostname cannot be resolved")
        {:error, "Sensor(#{sensor.id}) hostname cannot be resolved"}
      {:error, %{status_code: code}} ->
        IO.puts("Sensor(#{sensor.id}) returned status_code(#{code})")
        {:error, "Sensor(#{sensor.id}) returned status_code(#{code})"}
      {:error, reason} ->
        IO.puts("sensor(#{sensor.id}) actuation error: #{reason}")
        {:error, reason}
    end
  end

  # response when observation action didn't cause an error
  defp respond_success(conn, level, sensors) do

    # first - how many of our sensor actuations were a success or an error?
    grouped = Enum.group_by(sensors, fn (s) -> elem(s, 0) end, fn (s) -> true end)

    act_ok = length(Map.get(grouped, :ok, []))
    act_er = length(Map.get(grouped, :error, []))

    conn
    |> put_status(200)
    |> json(
        %{
          error: false,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          level: level,
          actuations: %{
            ok: act_ok,
            error: act_er
          }
        }
       )

  end

  # temporary data generation
  defp generate_actions() do
    Enum.map(1..:rand.uniform(10), fn (_) -> generate_action() end)
  end

  defp generate_action() do
    %{
      sensor: uuid4(),
      virtue: uuid4(),
      level: Enum.random(["off", "default", "low", "high", "adversarial"])
    }
  end

end