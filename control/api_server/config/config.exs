# This file is responsible for configuring your application
# and its dependencies with the aid of the Mix.Config module.
#
# This configuration file is loaded before any dependency and
# is restricted to this project.
use Mix.Config

# General application configuration
config :api_server,
  ecto_repos: [ApiServer.Repo],
  c2_kafka_topic: "api-server-control",
  sensor_kafka_bootstrap: ["kafka:9092"],
  client_kafka_bootstrap: ["kafka:9092"],
  ca_cert_file: "/app/certs/ca.pem"

# Configures the endpoint
config :api_server, ApiServer.Endpoint,
  url: [host: "api"],
  secret_key_base: "xtb19NAC0sCZ1RNjGUJVguTl7wsUKq/nsnjvx7Xxx/K3uP0aakfoQI/DYi3IH7M0",
  render_errors: [view: ApiServer.ErrorView, format: "json", accepts: ~w(json)],
  pubsub: [name: ApiServer.PubSub,
           adapter: Phoenix.PubSub.PG2],
  http: [protocol_options: [max_request_line_length: 8192, max_header_value_length: 8192]]

# Configures Elixir's Logger
config :logger, :console,
  format: "$time $metadata[$level] $message\n",
  metadata: [:request_id]

# Scheduler
config :api_server, ApiServer.Scheduler,
  jobs: [

    # heart beat every minute
    {"* * * * *",         {ApiServer, :heartbeat, []}},

    # check on non-responsive sensors every 2 minutes, and clean them out if
    # they're older than 5 minutes
    {"*/15 * * * *",       {ApiServer.DatabaseUtils, :prune_old_sensors, [15]}}
  ]

# Kafka connections
config :kafka_ex,

  # brokers
  brokers: [
    {"kafka", 9092}
  ],

  consumer_group: "kafka_ex",

  disable_default_worker: true,

  sync_timeout: 30000,

  max_restarts: 10,

  max_seconds: 60,

  commit_interval: 5_000,

  commit_threshold: 100,

  use_ssl: false,

  kafka_version: "0.9.0"



# Import environment specific config. This must remain at the bottom
# of this file so it overrides the configuration defined above.
import_config "#{Mix.env}.exs"
