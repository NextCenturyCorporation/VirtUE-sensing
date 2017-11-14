defmodule ApiServer.ValidateController do
  @moduledoc """
  Check or run diagnostic and security validations of a set of sensors.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """

  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_validate_action: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_validate_action when action in [:trigger]
  plug :extract_targeting when action in [:check, :trigger]


  @doc """
  Return the current validation state of the set of targeted sensors.
  This may include one or more sensors, from one or more Virtues.

  This is a _Plug.Conn handler/2_.

  The response JSON will look like:

  ```json
     {
       "targeting": { ... k/v map of targeting selectors ... },
       "error": false,
       "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
       "sensors": [
         {
           "sensor": "sensor UUID",
           "virtue": "ID of virtue containing sensor or observed by sensor",
           "name": "human readable sensor name"
           "compromised": false,
           "last-validated": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
           "last-validation-token": "UUID of last triggered and processed validation",
           "cross-validation": { ... map of last cross validation analysis data ... },
           "canary-validation": { ... map of last canary validation analysis data ... }
         },
         ...
       ]
     }
  ```

  ### Validations:

  ### Available Data:

    - conn::assigns::targeting - key/value propery map of target selectors

  ### Response:

    - HTTP 200 / JSON document describing the current state of the target sensors
  """
  def check(conn, _) do
    conn
      |> put_status(200)
      |> json(
            Map.merge(
              %{
                error: :false,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                sensors: generate_random_sensor_list()
              },
              summarize_targeting(conn.assigns)
            )
         )
  end


  @doc """
  Trigger a validation of the targeted sensors. The validation can be either "cross-validation"
  or "canary-validation". This may trigger validations for one or more sensors in one or more
  virtues.

  This is a _Plug.Conn handler/2_.

  The response will look like:

  ```json
     {
       "error": false,
       "virtues": [ ... list of virtue IDs effected ... ],
       "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
       "token": " UUID to track long running validation ",
       "validation": " canary-validate || cross-validate "
     }
  ```

  ### Validations:

    - `valid_validate_action` - value in validate action term set

  ### Available Data:

    - conn::assigns::targeting - key/value property map of target selectors
    - conn::assigns::validate_action - canary-validate or cross-validate

  ### Returns:

    - HTTP 200 / JSON document detailing results and tracking for validation
  """
  def trigger(conn, _) do

    conn
      |> put_status(200)
      |> json(
          Map.merge(
            %{
              error: :false,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              token: uuid4(),
              validation: conn.assigns.validate_action,
              virtues: Enum.map(1..:rand.uniform(10), fn(_) -> uuid4() end)
            },
            summarize_targeting(conn.assigns)
          )
         )
  end

  # Build a random list of between 1 to 10 sensors
  defp generate_random_sensor_list() do
    Enum.map(1..:rand.uniform(10), fn(_) -> generate_sensor() end)
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