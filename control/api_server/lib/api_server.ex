defmodule ApiServer do
  use Application
  alias :mnesia, as: Mnesia

  def version() do
    "2017.11.1"
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
    ]

    # spin up mnesia
    start_mnesia()

    # See https://hexdocs.pm/elixir/Supervisor.html
    # for other strategies and supported options
    opts = [strategy: :one_for_one, name: ApiServer.Supervisor]
    Supervisor.start_link(children, opts)
  end

  # Tell Phoenix to update the endpoint configuration
  # whenever the application is updated.
  def config_change(changed, _new, removed) do
    ApiServer.Endpoint.config_change(changed, removed)
    :ok
  end

  defp start_mnesia() do

    Mnesia.create_schema([node()])

    case Mnesia.start() do
      :ok ->
        IO.puts("Starting :mnesia on node(#{node()})")
      _ ->
        IO.puts("Encountered a problem starting :mnesia on node(#{node()})")
    end


    # Setup our Sensor tracking in Mnesia. Our database is versioned
    # with table transforms. A copy of the database is persisted to
    # disc via ETS.
    #
    #   Version 01
    #     - add id as default key
    #     - add sensor_id (indexed)
    #     - add virtue_id (indexed)
    #     - add username (indexed)
    #     - add address (indexed)
    #   Version 02
    #     - add registered_at timestamp
    #   Version 03
    #     - add port as companion to address
    case Mnesia.create_table(Sensor, [
      attributes: [:id, :sensor_id, :virtue_id, :username, :address, :timestamp, :port],
      disc_copies: [node()]
    ]) do

      {:atomic, :ok} ->

        IO.puts("  :: Sensor table created.")

        # normal table create / table did not yet exist
        Mnesia.add_table_index(Sensor, :sensor_id)
        Mnesia.add_table_index(Sensor, :virtue_id)
        Mnesia.add_table_index(Sensor, :username)
        Mnesia.add_table_index(Sensor, :address)

        IO.puts("  :: indexes added")

      {:aborted, {:already_exists, Sensor}} ->

        # table already exists, let's check the version
        case Mnesia.table_info(Sensor, :attributes) do

          # Version 03
          [:id, :sensor_id, :virtue_id, :username, :address, :timestamp, :port] ->
            IO.puts("  :: Sensor table Version 03 ready.")
            :ok

          # Version 02
          [:id, :sensor_id, :virtue_id, :username, :address, :timestamp] ->
            IO.puts("  :: Updating Sensor table: Version 02 => Version 03")
            Mnesia.transform_table(
              Sensor,
              fn ({Sensor, id, sensor_id, virtue_id, username, address, timestamp}) ->
                {Sensor, id, sensor_id, virtue_id, username, address, timestamp, 4000}
              end,
              [:id, :sensor_id, :virtue_id, :username, :address, :timestamp, :port]
            )
            IO.puts("  :: Sensor table updated.")

          # Version 01
          [:id, :sensor_id, :virtue_id, :username, :address] ->
            IO.puts("  :: Updating Sensor table: Version 01 => Version 03")
            Mnesia.transform_table(
              Sensor,
              fn ({Sensor, id, sensor_id, virtue_id, username, address}) ->
                {Sensor, id, sensor_id, virtue_id, username, address, DateTime.to_string(DateTime.utc_now()), 4000}
              end,
              [:id, :sensor_id, :virtue_id, :username, :address, :timestamp, :port]
            )
            IO.puts("  :: Sensor table updated.")

          other ->
            IO.puts("  :: Sensor table error #{other}")
            {:error, other}
        end
    end

    IO.puts("  :: Waiting for Mnesia to sync tables from disc")
    case Mnesia.wait_for_tables([Sensor], 10000) do
      :ok ->
        IO.puts("    - tables ready")
      {:timeout, _} ->
        IO.puts("    - synchronization timeout")
      {:error, reason} ->
        IO.puts("    - encountered an error while table syncing: #{reason}")
    end

    rec_count = Mnesia.table_info(Sensor, :size)
    IO.puts("  :: #{rec_count} Sensors currently registered")

  end
end
