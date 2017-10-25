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


     get "/virtues", StubController, :not_implemented
     get "/virtue/:id", StubController, :not_implemented

     ###############
     # NETWORK API #
     ###############
     put "/network/:cidr/observe/:level", StubController, :not_implemented, name: "network-cidr-observe"
     put "/network/virtue/:virtue/observe/:level", StubController, :not_implemented, name: "network-virtue-observe"
   end
end
