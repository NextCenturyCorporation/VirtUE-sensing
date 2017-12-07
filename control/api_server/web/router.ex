defmodule ApiServer.Router do
  use ApiServer.Web, :router


  pipeline :api do
    plug :accepts, ["json"]
    plug ApiServer.Plugs.Authenticate
  end

  pipeline :api_no_auth do
    plug :accepts, ["json"]
  end


  scope "/version", ApiServer do
    get "/", VersionController, :version
  end

  # API routes that do not require authentication
  scope "/api/v1", ApiServer do
    pipe_through :api_no_auth

    # server status/availability
    get "/ready", StatsController, :ready, name: "stats-ready"

    # sensor registration/sync/deregistration workflow
    put "/sensor/:sensor/register", RegistrationController, :register, name: "sensor-register"
    put "/sensor/:sensor/deregister", RegistrationController, :deregister, name: "sensor-deregister"
    put "/sensor/:sensor/sync", RegistrationController, :sync, name: "sensor-sync"

    # unauthenticated calls used during testing
    get "/sensor/:sensor/stream", StreamController, :stream, name: "sensor-stream"
    get "/user/:user/stream", StreamController, :stream, name: "user-stream"

  end

   # API routes requiring authentication
   scope "/api/v1", ApiServer do
     pipe_through :api

     # Static data routes
     get "/enum/observation/levels", EnumController, :observation_levels, name: "enum-observation-levels"
     get "/enum/log/levels", EnumController, :log_levels, name: "enum-log-levels"

     ###############
     # Control API #
     ###############
     get "/control/c2/channel", ControlController, :c2_channel, name: "control-c2-channel"

     ###############
     # NETWORK API #
     ###############
     put "/network/:cidr/observe/:level", ObserveController, :observe, name: "network-cidr-observe"
     put "/network/virtue/:virtue/observe/:level", ObserveController, :observe, name: "network-virtue-observe"
     put "/network/:cidr/trust/:action", TrustController, :trust, name: "network-cidr-trust"
     put "/network/virtue/:virtue/trust/:action", TrustController, :trust, name: "network-virtue-trust"
     get "/network/:cidr/stream", StreamController, :stream, name: "network-cidr-stream"
     get "/network/virtue/:virtue/stream", StreamController, :stream, name: "network-virtue-stream"
     get "/network/:cidr/inspect", InspectController, :inspect, name: "network-cidr-inspect"
     get "/network/virtue/:virtue/inspect", InspectController, :inspect, name: "network-virtue-inspect"
     get "/network/:cidr/validate/check", ValidateController, :check, name: "network-cidr-validate-check"
     get "/network/virtue/:virtue/validate/check", ValidateController, :check, name: "network-virtue-validate-check"
     put "/network/:cidr/validate/:action", ValidateController, :trigger, name: "network-cidr-validate-trigger"
     put "/network/virtue/:virtue/validate/:action", ValidateController, :trigger, name: "network-virtue-validate-trigger"

     ##############
     # VIRTUE API #
     ##############
     put "/virtue/:virtue/observe/:level", ObserveController, :observe, name: "virtue-observe"
     put "/virtue/user/:user/observe/:level", ObserveController, :observe, name: "virtue-user-observe"
     put "/virtue/:virtue/trust/:action", TrustController, :trust, name: "virtue-trust"
     put "/virtue/user/:user/trust/:action", TrustController, :trust, name: "virtue-user-trust"
     get "/virtue/:virtue/stream", StreamController, :stream, name: "virtue-stream"
     get "/virtue/user/:user/stream", StreamController, :stream, name: "virtue-user-stream"
     get "/virtue/:virtue/inspect", InspectController, :inspect, name: "virtue-inspect"
     get "/virtue/user/:user/inspect", InspectController, :inspect, name: "virtue-user-inspect"
     get "/virtue/:virtue/validate/check", ValidateController, :check, name: "virtue-validate-check"
     get "/virtue/user/:user/validate/check", ValidateController, :check, name: "virtue-user-validate-check"
     put "/virtue/:virtue/validate/:action", ValidateController, :trigger, name: "virtue-validate-trigger"
     put "/virtue/user/:user/validate/:action", ValidateController, :trigger, name: "virtue-user-validate-trigger"

     ##########
     # VM API #
     ##########
     put "/vm/virtue/:virtue/observe/:level", ObserveController, :observe, name: "vm-virtue-observe"
     put "/vm/user/:user/observe/:level", ObserveController, :observe, name: "vm-user-observe"
     put "/vm/address/:address/observe/:level", ObserveController, :observe, name: "vm-address-observe"
     put "/vm/virtue/:virtue/trust/:action", TrustController, :trust, name: "vm-virtue-trust"
     put "/vm/user/:user/trust/:action", TrustController, :trust, name: "vm-user-trust"
     put "/vm/address/:address/trust/:action", TrustController, :trust, name: "vm-address-trust"
     get "/vm/virtue/:virtue/stream", StreamController, :stream, name: "vm-virtue-stream"
     get "/vm/user/:user/stream", StreamController, :stream, name: "vm-user-stream"
     get "/vm/address/:address/stream", StreamController, :stream, name: "vm-address-stream"
     get "/vm/virtue/:virtue/inspect", InspectController, :inspect, name: "vm-virtue-inspect"
     get "/vm/user/:user/inspect", InspectController, :inspect, name: "vm-user-inspect"
     get "/vm/address/:address/inspect", InspectController, :inspect, name: "vm-address-inspect"
     get "/vm/virtue/:virtue/validate/check", ValidateController, :check, name: "vm-virtue-validate-check"
     get "/vm/user/:user/validate/check", ValidateController, :check, name: "vm-user-validate-check"
     get "/vm/address/:address/validate/check", ValidateController, :check, name: "vm-address-validate-check"
     put "/vm/virtue/:virtue/validate/:action", ValidateController, :trigger, name: "vm-virtue-validate-trigger"
     put "/vm/user/:user/validate/:action", ValidateController, :trigger, name: "vm-user-validate-trigger"
     put "/vm/address/:address/validate/:action", ValidateController, :trigger, name: "vm-address-validate-trigger"

     ###################
     # APPLICATION API #
     ###################
     put "/application/virtue/:virtue/observe/:level", ObserveController, :observe, name: "application-virtue-observe"
     put "/application/user/:user/observe/:level", ObserveController, :observe, name: "application-user-observe"
     put "/application/:application/observe/:level", ObserveController, :observe, name: "application-application-observe"
     put "/application/virtue/:virtue/user/:user/observe/:level", ObserveController, :observe, name: "application-virtue-user-observe"
     put "/application/virtue/:virtue/application/:application/observe/:level", ObserveController, :observe, name: "application-virtue-application-observe"
     put "/application/user/:user/application/:application/observe/:level", ObserveController, :observe, name: "application-user-application-observe"
     put "/application/virtue/:virtue/user/:user/application/:application/observe/:level", ObserveController, :observe, name: "application-virtue-user-application-observe"

     put "/application/virtue/:virtue/trust/:action", TrustController, :trust, name: "application-virtue-trust"
     put "/application/user/:user/trust/:action", TrustController, :trust, name: "application-user-trust"
     put "/application/:application/trust/:action", TrustController, :trust, name: "application-application-trust"
     put "/application/virtue/:virtue/user/:user/trust/:action", TrustController, :trust, name: "application-virtue-user-trust"
     put "/application/virtue/:virtue/application/:application/trust/:action", TrustController, :trust, name: "application-virtue-application-trust"
     put "/application/user/:user/application/:application/trust/:action", TrustController, :trust, name: "application-user-application-trust"
     put "/application/virtue/:virtue/user/:user/application/:application/trust/:action", TrustController, :trust, name: "application-virtue-user-application-trust"

     get "/application/virtue/:virtue/stream", StreamController, :stream, name: "application-virtue-stream"
     get "/application/user/:user/stream", StreamController, :stream, name: "application-user-stream"
     get "/application/:application/stream", StreamController, :stream, name: "application-application-stream"
     get "/application/virtue/:virtue/user/:user/stream", StreamController, :stream, name: "application-virtue-user-stream"
     get "/application/virtue/:virtue/application/:application/stream", StreamController, :stream, name: "application-virtue-application-stream"
     get "/application/user/:user/application/:application/stream", StreamController, :stream, name: "application-user-application-stream"
     get "/application/virtue/:virtue/user/:user/application/:application/stream", StreamController, :stream, name: "application-virtue-user-application-stream"

     get "/application/virtue/:virtue/inspect", InspectController, :inspect, name: "application-virtue-inspect"
     get "/application/user/:user/inspect", InspectController, :inspect, name: "application-user-inspect"
     get "/application/:application/inspect", InspectController, :inspect, name: "application-application-inspect"
     get "/application/virtue/:virtue/user/:user/inspect", InspectController, :inspect, name: "application-virtue-user-inspect"
     get "/application/virtue/:virtue/application/:application/inspect", InspectController, :inspect, name: "application-virtue-application-inspect"
     get "/application/user/:user/application/:application/inspect", InspectController, :inspect, name: "application-user-application-inspect"
     get "/application/virtue/:virtue/user/:user/application/:application/inspect", InspectController, :inspect, name: "application-virtue-user-application-inspect"

     get "/application/virtue/:virtue/validate/check", ValidateController, :check, name: "application-virtue-validate-check"
     get "/application/user/:user/validate/check", ValidateController, :check, name: "application-user-validate-check"
     get "/application/:application/validate/check", ValidateController, :check, name: "application-application-validate-check"
     get "/application/virtue/:virtue/user/:user/validate/check", ValidateController, :check, name: "application-virtue-user-validate-check"
     get "/application/virtue/:virtue/application/:application/validate/check", ValidateController, :check, name: "application-virtue-application-validate-check"
     get "/application/user/:user/application/:application/validate/check", ValidateController, :check, name: "application-user-application-validate-check"
     get "/application/virtue/:virtue/user/:user/application/:application/validate/check", ValidateController, :check, name: "application-virtue-user-application-validate-check"

     put "/application/virtue/:virtue/validate/:action", ValidateController, :trigger, name: "application-virtue-validate-trigger"
     put "/application/user/:user/validate/:action", ValidateController, :trigger, name: "application-user-validate-trigger"
     put "/application/:application/validate/:action", ValidateController, :trigger, name: "application-application-validate-trigger"
     put "/application/virtue/:virtue/user/:user/validate/:action", ValidateController, :trigger, name: "application-virtue-user-validate-trigger"
     put "/application/virtue/:virtue/application/:application/validate/:action", ValidateController, :trigger, name: "application-virtue-application-validate-trigger"
     put "/application/user/:user/application/:application/validate/:action", ValidateController, :trigger, name: "application-user-application-validate-trigger"
     put "/application/virtue/:virtue/user/:user/application/:application/validate/:action", ValidateController, :trigger, name: "application-virtue-user-application-validate-trigger"

     ################
     # RESOURCE API #
     ################
     put "/resource/:resource/observe/:level", ObserveController, :observe, name: "resource-observe"
     put "/resource/virtue/:virtue/observe/:level", ObserveController, :observe, name: "resource-virtue-observe"
     put "/resource/user/:user/observe/:level", ObserveController, :observe, name: "resource-user-observe"
     put "/resource/application/:application/observe/:level", ObserveController, :observe, name: "resource-application-observe"
     put "/resource/virtue/:virtue/user/:user/observe/:level", ObserveController, :observe, name: "resource-virtue-user-observe"
     put "/resource/virtue/:virtue/application/:application/observe/:level", ObserveController, :observe, name: "resource-virtue-application-observe"
     put "/resource/user/:user/application/:application/observe/:level", ObserveController, :observe, name: "resource-user-application-observe"
     put "/resource/virtue/:virtue/user/:user/application/:application/observe/:level", ObserveController, :observe, name: "resource-virtue-user-application-observe"

     put "/resource/:resource/trust/:action", TrustController, :trust, name: "resource-trust"
     put "/resource/virtue/:virtue/trust/:action", TrustController, :trust, name: "resource-virtue-trust"
     put "/resource/user/:user/trust/:action", TrustController, :trust, name: "resource-user-trust"
     put "/resource/application/:application/trust/:action", TrustController, :trust, name: "resource-application-trust"
     put "/resource/virtue/:virtue/user/:user/trust/:action", TrustController, :trust, name: "resource-virtue-user-trust"
     put "/resource/virtue/:virtue/application/:application/trust/:action", TrustController, :trust, name: "resource-virtue-application-trust"
     put "/resource/user/:user/application/:application/trust/:action", TrustController, :trust, name: "resource-user-application-trust"
     put "/resource/virtue/:virtue/user/:user/application/:application/trust/:action", TrustController, :trust, name: "resource-virtue-user-application-trust"

     get "/resource/:resource/stream", StreamController, :stream, name: "resource-stream"
     get "/resource/virtue/:virtue/stream", StreamController, :stream, name: "resource-virtue-stream"
     get "/resource/user/:user/stream", StreamController, :stream, name: "resource-user-stream"
     get "/resource/application/:application/stream", StreamController, :stream, name: "resource-application-stream"
     get "/resource/virtue/:virtue/user/:user/stream", StreamController, :stream, name: "resource-virtue-user-stream"
     get "/resource/virtue/:virtue/application/:application/stream", StreamController, :stream, name: "resource-virtue-application-stream"
     get "/resource/user/:user/application/:application/stream", StreamController, :stream, name: "resource-user-application-stream"
     get "/resource/virtue/:virtue/user/:user/application/:application/stream", StreamController, :stream, name: "resource-virtue-user-application-stream"

     get "/resource/:resource/inspect", InspectController, :inspect, name: "resource-inspect"
     get "/resource/virtue/:virtue/inspect", InspectController, :inspect, name: "resource-virtue-inspect"
     get "/resource/user/:user/inspect", InspectController, :inspect, name: "resource-user-inspect"
     get "/resource/application/:application/inspect", InspectController, :inspect, name: "resource-application-inspect"
     get "/resource/virtue/:virtue/user/:user/inspect", InspectController, :inspect, name: "resource-virtue-user-inspect"
     get "/resource/virtue/:virtue/application/:application/inspect", InspectController, :inspect, name: "resource-virtue-application-inspect"
     get "/resource/user/:user/application/:application/inspect", InspectController, :inspect, name: "resource-user-application-inspect"
     get "/resource/virtue/:virtue/user/:user/application/:application/inspect", InspectController, :inspect, name: "resource-virtue-user-application-inspect"

     get "/resource/:resource/validate/check", ValidateController, :check, name: "resource-validate-check"
     get "/resource/virtue/:virtue/validate/check", ValidateController, :check, name: "resource-virtue-validate-check"
     get "/resource/user/:user/validate/check", ValidateController, :check, name: "resource-user-validate-check"
     get "/resource/application/:application/validate/check", ValidateController, :check, name: "resource-application-validate-check"
     get "/resource/virtue/:virtue/user/:user/validate/check", ValidateController, :check, name: "resource-virtue-user-validate-check"
     get "/resource/virtue/:virtue/application/:application/validate/check", ValidateController, :check, name: "resource-virtue-application-validate-check"
     get "/resource/user/:user/application/:application/validate/check", ValidateController, :check, name: "resource-user-application-validate-check"
     get "/resource/virtue/:virtue/user/:user/application/:application/validate/check", ValidateController, :check, name: "resource-virtue-user-application-validate-check"

     put "/resource/:resource/validate/:action", ValidateController, :trigger, name: "resource-validate-trigger"
     put "/resource/virtue/:virtue/validate/:action", ValidateController, :trigger, name: "resource-virtue-validate-trigger"
     put "/resource/user/:user/validate/:action", ValidateController, :trigger, name: "resource-user-validate-trigger"
     put "/resource/application/:application/validate/:action", ValidateController, :trigger, name: "resource-application-validate-trigger"
     put "/resource/virtue/:virtue/user/:user/validate/:action", ValidateController, :trigger, name: "resource-virtue-user-validate-trigger"
     put "/resource/virtue/:virtue/application/:application/validate/:action", ValidateController, :trigger, name: "resource-virtue-application-validate-trigger"
     put "/resource/user/:user/application/:application/validate/:action", ValidateController, :trigger, name: "resource-user-application-validate-trigger"
     put "/resource/virtue/:virtue/user/:user/application/:application/validate/:action", ValidateController, :trigger, name: "resource-virtue-user-application-validate-trigger"

     ############
     # USER API #
     ############
     put "/user/:user/observe/:level", ObserveController, :observe, name: "user-observe"
     put "/user/:user/trust/:action", TrustController, :trust, name: "user-trust"
     get "/user/:user/inspect", InspectController, :inspect, name: "user-inspect"
     get "/user/:user/validate/check", ValidateController, :check, name: "user-validate-check"
     put "/user/:user/validate/:action", ValidateController, :trigger, name: "user-validate-trigger"

     ##############
     # SENSOR API #
     ##############
     get "/sensor/:sensor/configure", ConfigureController, :review, name: "sensor-configure-get"
     put "/sensor/:sensor/configure", ConfigureController, :configure, name: "sensor-configure-set"
     get "/sensor/:sensor/validate/check", ValidateController, :check, name: "sensor-validate-check"
     put "/sensor/:sensor/validate/:action", ValidateController, :trigger, name: "sensor-validate-trigger"
     get "/sensor/:sensor/inspect", InspectController, :inspect, name: "sensor-inspect"

  end
end
