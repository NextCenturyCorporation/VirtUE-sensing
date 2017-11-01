defmodule ApiServer.TrustController do

  use ApiServer.Web, :controller

  import ApiServer.ValidationPlug, only: [valid_trust_action: 2]
  import ApiServer.ExtractionPlug, only: [extract_targeting: 2]
  import UUID, only: [uuid4: 0]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :valid_trust_action when action in [:trust]
  plug :extract_targeting when action in [:trust]

  # Run certificate validations for all sensors observing virtues, applications,
  # users, or resources within the target selectors. This will run certificate
  # validations on zero or more sensors.
  #
  # The JSON response from this will look like:
  #
  #   {
  #     "error": false,
  #     "targeting": { ... k/v of targeting selectors ... },
  #     "action": "validate",
  #     "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #     "validations": [
  #       {
  #         "sensor": uuid,
  #         "virtue": uuid,
  #         "certificate": {
  #           "not_valid_before": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #           "not_valid_after": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
  #           "CN": "sensor-name-and-id",
  #           "key_type": "RSA Public Key (4096 bit)",
  #           "fingerprint": "bunch of base16 octets",
  #           "chain_of_trust": ["intermediate-cert-name", ..., "root-cert-name"]
  #         }
  #       }
  #     ]
  #   }
  #
  # Available data:
  #   - conn::assigns::targeting - key/value propery map of target selectors
  #
  # Response:
  #   - JSON document describing the certificate validations of scoped sensors
  #
  def trust(%Plug.Conn{params: %{"action" => "validate"}} = conn, _) do

    conn
      |> put_status(200)
      |> json(
          Map.merge(
            %{
              error: :false,
              action: "validate",
              timestamp: DateTime.to_string(DateTime.utc_now()),
              validations: generate_validations()
            },
            summarize_targeting(conn.assigns)
          )
         )
  end

  def trust(%Plug.Conn{params: %{"action" => "invalidate"}} = conn, _) do

    conn
      |> put_status(200)
      |> json(
          Map.merge(
            %{
              error: :false,
              action: "invalidate",
              timestamp: DateTime.to_string(DateTime.utc_now()),
              invalidations: generate_invalidations()
            },
            summarize_targeting(conn.assigns)
          )
         )
  end

  defp generate_invalidations() do
    Enum.map(1..:rand.uniform(10), fn(x) -> generate_invalidation() end)
  end

  defp generate_validations() do
    Enum.map(1..:rand.uniform(10), fn(x) -> generate_validation() end)
  end

  defp generate_validation() do
    %{
      sensor: uuid4(),
      virtue: uuid4(),
      certificate: %{
        not_valid_before: DateTime.to_string(DateTime.utc_now()),
        not_valid_after: DateTime.to_string(DateTime.utc_now()),
        cn: uuid4(),
        key_type: "RSA Public Key (4096 bit)",
        fingerprint: "07:3F:E2:BA:74:E2:57:68:83:83:32:98:74:D4:B7:AA:8D:CD:9E:FE:52:A6:64:B5:5B:80:19:F1:6F:5F:A3:94",
        chain_of_trust: Enum.map(1..:rand.uniform(5), fn(x) -> uuid4() end)
      }
    }
  end

  defp generate_invalidation() do
    %{
      sensor: uuid4(),
      virtue: uuid4(),
      certificate: %{
        "cn": uuid4(),
        "fingerprint": "07:3F:E2:BA:74:E2:57:68:83:83:32:98:74:D4:B7:AA:8D:CD:9E:FE:52:A6:64:B5:5B:80:19:F1:6F:5F:A3:94"
      }
    }
  end

end