defmodule ApiServer do
  use Application
  alias :mnesia, as: Mnesia
  use Retry

  def heartbeat() do
    IO.puts("[heartbeat] #{DateTime.to_string(DateTime.utc_now())}")
    ApiServer.ControlUtils.heartbeat()
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

    # spin up mnesia
    start_mnesia()


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

  defp start_mnesia() do

    Mnesia.create_schema([node()])

    case Mnesia.start() do
      :ok ->
        IO.puts("Starting :mnesia on node(#{node()})")
      _ ->
        IO.puts("Encountered a problem starting :mnesia on node(#{node()})")
    end

    IO.puts("  :: Waiting for Mnesia to sync tables from disc")
    case Mnesia.wait_for_tables([PKIKey], 10000) do
      :ok ->
        IO.puts("    + tables ready")
      {:timeout, _} ->
        IO.puts("    - synchronization timeout")
      {:error, reason} ->
        IO.puts("    - encountered an error while table syncing: #{reason}")
    end




    # When updates to the data model for sensors are done, they
    # need to be reflected in a number of places. See the API Server
    # README for more info:
    #
    #   https://github.com/twosixlabs/savior/tree/master/control/api_server#data-models
    #
    # Setup our Public/Private key tracking in Mnesia. Our database is versioned
    # with table transforms. A copy of the database is persisted to
    # disc via ETS.
    #
    #   Version 01
    #     - add hostname
    #     - add port
    #     - add challenge JSON
    #     - add pub/priv key algorithm
    #     - add pub/priv key size
    #     - add CSR
    #     - add private key hash
    #     - add public key
    case Mnesia.create_table(PKIKey, [
      attributes: [:id, :hostname, :port, :challenge, :algo, :size, :csr, :private_key_hash, :public_key],
      disc_copies: [node()]
    ]) do

      {:atomic, :ok} ->

        IO.puts("  :: PKIKey table created.")

        # normal table create / table did not yet exist
        Mnesia.add_table_index(PKIKey, :csr)
        Mnesia.add_table_index(PKIKey, :public_key)
        Mnesia.add_table_index(PKIKey, :private_key_hash)

        IO.puts("  :: indexes added")

      {:aborted, {:already_exists, PKIKey}} ->

        # table already exists, let's check the version
        case Mnesia.table_info(PKIKey, :attributes) do

          # Version 01
          [:id, :hostname, :port, :challenge, :algo, :size, :csr, :private_key_hash, :public_key] ->
            IO.puts("  :: PKIKey table Version 01 ready.")


          other ->
            IO.puts("  :: PKIKey table error #{other}")
            {:error, other}
        end
    end

    pki_keys_rec_count = Mnesia.table_info(PKIKey, :size)
    IO.puts("  :: #{pki_keys_rec_count } PKI Keys currently registered")

  end
end
