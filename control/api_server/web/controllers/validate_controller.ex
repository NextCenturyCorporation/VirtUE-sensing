defmodule ApiServer.ValidateController do
  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_validate_action: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_validate_action when action in [:trigger]
  plug :extract_targeting when action in [:check, :trigger]

  def check(conn, _) do
    conn
      |> put_status(200)
      |> json(
          %{
            targeting: summarize_targeting(conn.assigns),
            error: :false,
            timestamp: DateTime.to_string(DateTime.utc_now()),
            sensors: generate_random_sensor_list()
          }
         )
  end

  def trigger(conn, _) do
    json conn_with_status(conn, 501), conn_json(conn, 501)
  end

  defp conn_with_status(conn, stat) do
    conn
    |> put_status(stat)
  end

  defp conn_json(conn, code) do
    handled_by = controller_module(conn)
    %{
      path: current_path(conn),
      action: action_name(conn),
      status_code: code,
      msg: "Not Implemented - Handled by #{handled_by}",
      error: :true
    }
  end

  # Build a random list of between 1 to 10 sensors
  defp generate_random_sensor_list() do
    Enum.map(1..:rand.uniform(10), fn(x) -> generate_sensor() end)
  end

  # build a random sensor
  defp generate_sensor() do
    %{
      "sensor" => uuid4(),
      "virtue" => uuid4(),
      "name" => Enum.random(["snort", "tripwire", "ptrace"]),
      "compromised" => :false,
      "last-validated" => DateTime.to_string(DateTime.utc_now()),
      "cross-validation": %{},
      "canary-validation": %{}
    }
  end
end