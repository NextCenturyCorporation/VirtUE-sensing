# This file is responsible for configuring your application
# and its dependencies with the aid of the Mix.Config module.
#
# This configuration file is loaded before any dependency and
# is restricted to this project.
use Mix.Config

# General application configuration
config :api_server,
  ecto_repos: [ApiServer.Repo]

# Configures the endpoint
config :api_server, ApiServer.Endpoint,
  url: [host: "localhost"],
  secret_key_base: "xtb19NAC0sCZ1RNjGUJVguTl7wsUKq/nsnjvx7Xxx/K3uP0aakfoQI/DYi3IH7M0",
  render_errors: [view: ApiServer.ErrorView, format: "json", accepts: ~w(json)],
  pubsub: [name: ApiServer.PubSub,
           adapter: Phoenix.PubSub.PG2]

# Configures Elixir's Logger
config :logger, :console,
  format: "$time $metadata[$level] $message\n",
  metadata: [:request_id]

# Import environment specific config. This must remain at the bottom
# of this file so it overrides the configuration defined above.
import_config "#{Mix.env}.exs"
