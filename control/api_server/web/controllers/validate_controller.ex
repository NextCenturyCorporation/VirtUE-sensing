defmodule ApiServer.ValidateController do
  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_validate_action: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_validate_action when action in [:trigger]
  plug :extract_targeting when action in [:check, :trigger]

  # Return the current validation state of the set of targeted sensors.
  # This may include one or more sensors, from one or more Virtues.
  #
  # The response JSON will look like:
  #
  #   {
  #     "targeting": { ... k/v map of targeting selectors ... },
  #     "error": false,
  #     "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #     "sensors": [
  #       {
  #         "sensor": "sensor UUID",
  #         "virtue": "ID of virtue containing sensor or observed by sensor",
  #         "name": "human readable sensor name"
  #         "compromised": false,
  #         "last-validated": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #         "last-validation-token": "UUID of last triggered and processed validation",
  #         "cross-validation": { ... map of last cross validation analysis data ... },
  #         "canary-validation": { ... map of last canary validation analysis data ... }
  #       },
  #       ...
  #     ]
  #   }
  #
  # Available Data:
  #   - conn::assigns::targeting - key/value propery map of target selectors
  #
  # Response:
  #   - JSON document describing the current state of the target sensors
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

  # Trigger a validation of the targeted sensors. The validation can be either "cross-validation"
  # or "canary-validation". This may trigger validations for one or more sensors in one or more
  # virtues.
  #
  # The response will look like:
  #
  #   {
  #     "error": false,
  #     "virtues": [ ... list of virtue IDs effected ... ],
  #     "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #     "token": " UUID to track long running validation ",
  #     "validation": " canary-validate || cross-validate "
  #   }
  #
  # Available Data:
  #   - conn::assigns::targeting - key/value property map of target selectors
  #   - conn::assigns::validate_action - canary-validate or cross-validate
  def trigger(conn, _) do

    conn
      |> put_status(200)
      |> json(
          %{
            error: :false,
            timestamp: DateTime.to_string(DateTime.utc_now()),
            token: uuid4(),
            validation: conn.assigns.validate_action,
            virtues: Enum.map(1..:rand.uniform(10), fn(x) -> uuid4() end)
          }
         )
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
      "last-validation-token": uuid4(),
      "cross-validation": %{},
      "canary-validation": %{}
    }
  end
end