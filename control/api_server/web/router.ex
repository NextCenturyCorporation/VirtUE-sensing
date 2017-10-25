defmodule ApiServer.Router do
  use ApiServer.Web, :router

#  pipeline :browser do
#    plug :accepts, ["html"]
#    plug :fetch_session
#    plug :fetch_flash
#    plug :protect_from_forgery
#    plug :put_secure_browser_headers
#  end

  pipeline :api do
    plug :accepts, ["json"]
    plug ApiServer.Plugs.Authenticate
  end

#  scope "/", ApiServer do
#    pipe_through :browser # Use the default browser stack
#
#    get "/", PageController, :index
#  end

   # Other scopes may use custom stacks.
   scope "/api/v1", ApiServer do
     pipe_through :api

     # Ping/ack endpoints
     get "/qotds", StatsController, :qotds
     get "/qotd/:id", StatsController, :qotd

     # Static data routes
     get "/enum/observation/levels", EnumController, :observation_levels, name: "enum-observation-levels"
     get "/enum/log/levels", EnumController, :log_levels, name: "enum-log-levels"

     get "/virtues", StubController, :not_implemented
     get "/virtue/:id", StubController, :not_implemented

     ###############
     # NETWORK API #
     ###############
     put "/network/:cidr/observe/:level", StubController, :not_implemented, name: "network-cidr-observe"
     put "/network/virtue/:virtue/observe/:level", StubController, :not_implemented, name: "network-virtue-observe"
     put "/network/:cidr/trust/:action", StubController, :not_implemented, name: "network-cidr-trust"
     put "/network/virtue/:virtue/trust/:action", StubController, :not_implemented, name: "network-virtue-trust"
     get "/network/:cidr/stream", StubController, :not_implemented, name: "network-cidr-stream"
     get "/network/virtue/:virtue/stream", StubController, :not_implemented, name: "network-virtue-stream"
     get "/network/:cidr/inspect", StubController, :not_implemented, name: "network-cidr-inspect"
     get "/network/virtue/:virtue/inspect", StubController, :not_implemented, name: "network-virtue-inspect"
     get "/network/:cidr/validate/check", StubController, :not_implemented, name: "network-cidr-validate-check"
     get "/network/virtue/:virtue/validate/check", StubController, :not_implemented, name: "network-virtue-validate-check"
     put "/network/:cidr/validate/:action", StubController, :not_implemented, name: "network-cidr-validate-trigger"
     put "/network/virtue/:virtue/validate/:action", StubController, :not_implemented, name: "network-virtue-validate-trigger"

     ##############
     # VIRTUE API #
     ##############
     put "/virtue/:virtue/observe/:level", StubController, :not_implemented, name: "virtue-observe"
     put "/virtue/user/:user/observe/:level", StubController, :not_implemented, name: "virtue-user-observe"
     put "/virtue/:virtue/trust/:action", StubController, :not_implemented, name: "virtue-trust"
     put "/virtue/user/:user/trust/:action", StubController, :not_implemented, name: "virtue-user-trust"
     get "/virtue/:virtue/stream", StubController, :not_implemented, name: "virtue-stream"
     get "/virtue/user/:user/stream", StubController, :not_implemented, name: "virtue-user-stream"
     get "/virtue/:virtue/inspect", StubController, :not_implemented, name: "virtue-inspect"
     get "/virtue/user/:user/inspect", StubController, :not_implemented, name: "virtue-user-inspect"
     get "/virtue/:virtue/validate/check", StubController, :not_implemented, name: "virtue-validate-check"
     get "/virtue/user/:user/validate/check", StubController, :not_implemented, name: "virtue-user-validate-check"
     put "/virtue/:virtue/validate/:action", StubController, :not_implemented, name: "virtue-validate-trigger"
     put "/virtue/user/:user/validate/:action", StubController, :not_implemented, name: "virtue-user-validate-trigger"

  end
end
