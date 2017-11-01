defmodule ApiServer.InspectController do
  use ApiServer.Web, :controller

  import UUID, only: [uuid4: 0]

  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :extract_targeting when action in [:inspect]

  # Return the set of sensors observing the resources described
  # by the targeting selectors. This is a list of zero or more
  # sensors.
  #
  # The response will looke like:
  #
  #   {
  #     "targeting": { ... k/v map of targeting selectors ... },
  #     "error": false,
  #     "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #     "sensors": [
  #       {
  #         "sensor": sensor uuid,
  #         "virtue": virtue containing sensor/observed by sensor,
  #         "state": ( active || inactive ),
  #         "name": sensor human readable name
  #       }
  #     ]
  #   }
  def inspect(conn, _) do
    conn
      |> put_status(200)
      |> json(
          Map.merge(
            %{
              "error": :false,
              "timestamp": DateTime.to_string(DateTime.utc_now()),
              "sensors": generate_random_sensor_list()
            },
            summarize_targeting(conn.assigns)
          )
         )

  end

  defp generate_random_sensor_list() do
    Enum.map(1..:rand.uniform(10), fn (x) -> generate_random_sensor() end)
  end

  defp generate_random_sensor() do
    %{
      "sensor": uuid4(),
      "virtue": uuid4(),
      "state": Enum.random(["active", "inactive"]),
      "name": Enum.random(["snort", "tripwire", "ptrace"])
    }
  end

end