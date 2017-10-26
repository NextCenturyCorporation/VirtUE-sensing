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

     ##########
     # VM API #
     ##########
     put "/vm/virtue/:virtue/observe/:level", StubController, :not_implemented, name: "vm-virtue-observe"
     put "/vm/user/:user/observe/:level", StubController, :not_implemented, name: "vm-user-observe"
     put "/vm/address/:address/observe/:level", StubController, :not_implemented, name: "vm-address-observe"
     put "/vm/virtue/:virtue/trust/:action", StubController, :not_implemented, name: "vm-virtue-trust"
     put "/vm/user/:user/trust/:action", StubController, :not_implemented, name: "vm-user-trust"
     put "/vm/address/:address/trust/:action", StubController, :not_implemented, name: "vm-address-trust"
     get "/vm/virtue/:virtue/stream", StubController, :not_implemented, name: "vm-virtue-stream"
     get "/vm/user/:user/stream", StubController, :not_implemented, name: "vm-user-stream"
     get "/vm/address/:address/stream", StubController, :not_implemented, name: "vm-address-stream"
     get "/vm/virtue/:virtue/inspect", StubController, :not_implemented, name: "vm-virtue-inspect"
     get "/vm/user/:user/inspect", StubController, :not_implemented, name: "vm-user-inspect"
     get "/vm/address/:address/inspect", StubController, :not_implemented, name: "vm-address-inspect"
     get "/vm/virtue/:virtue/validate/check", StubController, :not_implemented, name: "vm-virtue-validate-check"
     get "/vm/user/:user/validate/check", StubController, :not_implemented, name: "vm-user-validate-check"
     get "/vm/address/:address/validate/check", StubController, :not_implemented, name: "vm-address-validate-check"
     put "/vm/virtue/:virtue/trust/validate/:action", StubController, :not_implemented, name: "vm-virtue-validate-trigger"
     put "/vm/user/:user/validate/:action", StubController, :not_implemented, name: "vm-user-validate-trigger"
     put "/vm/address/:address/validate/:action", StubController, :not_implemented, name: "vm-address-validate-trigger"

     ###################
     # APPLICATION API #
     ###################
     put "/application/virtue/:virtue/observe/:level", StubController, :not_implemented, name: "application-virtue-observe"
     put "/application/user/:user/observe/:level", StubController, :not_implemented, name: "application-user-observe"
     put "/application/:application/observe/:level", StubController, :not_implemented, name: "application-application-observe"
     put "/application/virtue/:virtue/user/:user/observe/:level", StubController, :not_implemented, name: "application-virtue-user-observe"
     put "/application/virtue/:virtue/applicaiton/:application/observe/:level", StubController, :not_implemented, name: "application-virtue-application-observe"
     put "/application/user/:user/application/:application/observe/:level", StubController, :not_implemented, name: "application-user-application-observe"
     put "/application/virtue/:virtue/user/:user/application/:application/observe/:level", StubController, :not_implemented, name: "application-virtue-user-application-observe"

     put "/application/virtue/:virtue/trust/:action", StubController, :not_implemented, name: "application-virtue-trust"
     put "/application/user/:user/trust/:action", StubController, :not_implemented, name: "application-user-trust"
     put "/application/:application/trust/:action", StubController, :not_implemented, name: "application-application-trust"
     put "/application/virtue/:virtue/user/:user/trust/:action", StubController, :not_implemented, name: "application-virtue-user-trust"
     put "/application/virtue/:virtue/applicaiton/:application/trust/:action", StubController, :not_implemented, name: "application-virtue-application-trust"
     put "/application/user/:user/application/:application/trust/:action", StubController, :not_implemented, name: "application-user-application-trust"
     put "/application/virtue/:virtue/user/:user/application/:application/trust/:action", StubController, :not_implemented, name: "application-virtue-user-application-trust"

     get "/application/virtue/:virtue/stream", StubController, :not_implemented, name: "application-virtue-stream"
     get "/application/user/:user/stream", StubController, :not_implemented, name: "application-user-stream"
     get "/application/:application/stream", StubController, :not_implemented, name: "application-application-stream"
     get "/application/virtue/:virtue/user/:user/stream", StubController, :not_implemented, name: "application-virtue-user-stream"
     get "/application/virtue/:virtue/applicaiton/:application/stream", StubController, :not_implemented, name: "application-virtue-application-stream"
     get "/application/user/:user/application/:application/stream", StubController, :not_implemented, name: "application-user-application-stream"
     get "/application/virtue/:virtue/user/:user/application/:application/stream", StubController, :not_implemented, name: "application-virtue-user-application-stream"

     get "/application/virtue/:virtue/inspect", StubController, :not_implemented, name: "application-virtue-inspect"
     get "/application/user/:user/inspect", StubController, :not_implemented, name: "application-user-inspect"
     get "/application/:application/inspect", StubController, :not_implemented, name: "application-application-inspect"
     get "/application/virtue/:virtue/user/:user/inspect", StubController, :not_implemented, name: "application-virtue-user-inspect"
     get "/application/virtue/:virtue/applicaiton/:application/inspect", StubController, :not_implemented, name: "application-virtue-application-inspect"
     get "/application/user/:user/application/:application/inspect", StubController, :not_implemented, name: "application-user-application-inspect"
     get "/application/virtue/:virtue/user/:user/application/:application/inspect", StubController, :not_implemented, name: "application-virtue-user-application-inspect"

     get "/application/virtue/:virtue/validate/check", StubController, :not_implemented, name: "application-virtue-validate-check"
     get "/application/user/:user/validate/check", StubController, :not_implemented, name: "application-user-validate-check"
     get "/application/:application/validate/check", StubController, :not_implemented, name: "application-application-validate-check"
     get "/application/virtue/:virtue/user/:user/validate/check", StubController, :not_implemented, name: "application-virtue-user-validate-check"
     get "/application/virtue/:virtue/applicaiton/:application/validate/check", StubController, :not_implemented, name: "application-virtue-application-validate-check"
     get "/application/user/:user/application/:application/validate/check", StubController, :not_implemented, name: "application-user-application-validate-check"
     get "/application/virtue/:virtue/user/:user/application/:application/validate/check", StubController, :not_implemented, name: "application-virtue-user-application-validate-check"

     put "/application/virtue/:virtue/validate/:action", StubController, :not_implemented, name: "application-virtue-validate-trigger"
     put "/application/user/:user/validate/:action", StubController, :not_implemented, name: "application-user-validate-trigger"
     put "/application/:application/validate/:action", StubController, :not_implemented, name: "application-application-validate-trigger"
     put "/application/virtue/:virtue/user/:user/validate/:action", StubController, :not_implemented, name: "application-virtue-user-validate-trigger"
     put "/application/virtue/:virtue/applicaiton/:application/validate/:action", StubController, :not_implemented, name: "application-virtue-application-validate-trigger"
     put "/application/user/:user/application/:application/validate/:action", StubController, :not_implemented, name: "application-user-application-validate-trigger"
     put "/application/virtue/:virtue/user/:user/application/:application/validate/:action", StubController, :not_implemented, name: "application-virtue-user-application-validate-trigger"

  end
end
