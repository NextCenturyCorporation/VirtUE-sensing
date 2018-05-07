defmodule ApiServer do
  use Application
  use Retry

  def heartbeat() do
    IO.puts("[heartbeat] #{DateTime.to_string(DateTime.utc_now())}")
    ApiServer.ControlUtils.heartbeat()
  end

  def sensor_status_heartbeat() do
    IO.puts("[sensor status heartbeat] #{DateTime.to_string(DateTime.utc_now())}")
    ApiServer.ControlUtils.sensor_status()
  end

  def version() do
    "2018.01.08"
  end

  # See https://hexdocs.pm/elixir/Application.html
  # for more information on OTP Applications
  def start(_type, _args) do
    import Supervisor.Spec

    # Define workers and child supervisors to be supervised
    children = [
      # Start the Ecto repository
      supervisor(ApiServer.Repo, []),

      # Start the endpoint when the application starts
      supervisor(ApiServer.Endpoint, []),
      # Start your own worker by calling: ApiServer.Worker.start_link(arg1, arg2, arg3)
      # worker(ApiServer.Worker, [arg1, arg2, arg3]),
      worker(ApiServer.Scheduler, [])
    ]


    # See https://hexdocs.pm/elixir/Supervisor.html
    # for other strategies and supported options
    opts = [strategy: :one_for_one, name: ApiServer.Supervisor]
    sup = Supervisor.start_link(children, opts)

    # spin up kafka connection
    start_kafkaex()

    sup
  end

  # Tell Phoenix to update the endpoint configuration
  # whenever the application is updated.
  def config_change(changed, _new, removed) do
    ApiServer.Endpoint.config_change(changed, removed)
    :ok
  end

  defp start_kafkaex() do

    IO.puts("  :: Waiting for Kafka")
    result = retry with: exp_backoff |> cap(1_000) |> expiry(20_000) do

      case KafkaEx.create_worker(:kafka_ex) do
        {:ok, _} ->
          IO.puts("    + Kafka is ready")
          {:ok, "kafka ready"}
        {:error, e} ->
          IO.puts("    ... Kafka not ready")
          {:error, e}
      end
    end

  end


end
