defmodule ApiServer.ObserveController do
  use ApiServer.Web, :controller
  import ApiServer.ValidationPlug, only: [valid_observe_level: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_observe_level when action in [:observe]
  plug :extract_targeting when action in [:observe]

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

  defp generate_actions() do
    Enum.map(1..:rand.uniform(10), fn (x) -> generate_action() end)
  end

  defp generate_action() do
    %{
      sensor: uuid4(),
      virtue: uuid4(),
      level: Enum.random(["off", "default", "low", "high", "adversarial"])
    }
  end

end