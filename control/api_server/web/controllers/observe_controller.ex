defmodule ApiServer.ObserveController do
  @moduledoc """
  Set or retrieve the observation level for a set of sensors.
  """

  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_observe_level: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_observe_level when action in [:observe]
  plug :extract_targeting when action in [:observe]

  @doc """
  Set the observation level of the sensors within the targeting
  spec.

  This may alter the level or runtime of one or more sensors,
  including halting or starting sensors.

  Available data:
    - conn::assigns::targeting - key/value propery map of target selectors

  Returns:
    - HTTP/200 - JSON document with observation results
  """
  def observe(%Plug.Conn{params: %{"level" => level}} = conn, _) do

    conn
      |> put_status(200)
      |> json(
          Map.merge(
            %{
              error: :false,
              level: level,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              actions: generate_actions()
            },
            summarize_targeting(conn.assigns)
          )
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