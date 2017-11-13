defmodule ApiServer.Scheduler do
  use Quantum.Scheduler, otp_app: :api_server
end