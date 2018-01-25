defmodule ApiServer.Sensor do
  @moduledoc """
  Schema for working with sensor records in the database.

  """
  use Ecto.Schema
  import Ecto.Query
  import UUID, only: [uuid4: 0]

  schema "sensors" do

    # required fields at sensor creation
    field :sensor_id, :string
    field :virtue_id, :string
    field :username, :string
    field :address, :string
    field :port, :integer
    field :public_key, :string
    field :kafka_topic, :string

    # flags set during registration and authentication cycles
    field :has_certificates, :boolean, default: false
    field :has_registered, :boolean, default: false

    # relationships
    belongs_to :component, ApiServer.Component
    belongs_to :configuration, ApiServer.Configuration

    # timestamp tracking (the default value doesn't do what you think:
    # this is called at compile time, so the default will be static,
    # which is enough for now - we just want a valid value to start)
    field :last_sync_at, :utc_datetime, default: DateTime.utc_now
    timestamps()

  end

  def changeset(sensor, params \\ %{}) do
    sensor
    |> Ecto.Changeset.cast(params, [:sensor_id, :virtue_id, :username, :address, :port, :public_key, :kafka_topic, :last_sync_at, :has_registered, :has_certificates])
    |> Ecto.Changeset.validate_required([:sensor_id, :virtue_id, :username, :address, :port])
    |> Ecto.Changeset.validate_number(:port, greater_than: 0)
    |> Ecto.Changeset.validate_number(:port, less_than: 65536)
  end

  @doc """
  Create and optionally save a new sensor. Roughly an alias to ApiServer.Sensor.changeset/2

  Call with `save: true` to automatically wrap in a DB insert:

    case ApiServer.Sensor.create(%{}, save: true) do
      {:ok, sensor} ->
      {:error, changeset} ->
    end
  """
  def create(params, opts \\ []) do

    # add some possible auto-generated fields
    create_params = Map.merge(%{last_sync_at: DateTime.utc_now(), kafka_topic: uuid4()}, params)

    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.insert(ApiServer.Sensor.changeset(%ApiServer.Sensor{}, create_params))
      :false ->
        ApiServer.Sensor.changeset(%ApiServer.Sensor{}, params)
    end
  end

  @doc """
  Update an existing sensor using a changeset, and optionally save to the database
  with the `save: true` flag.

    case ApiServer.Sensor.update(%ApiServer.Sensor{}, %{...}, save: true) do
      {:ok, sensor} ->
      {:error, changeset} ->
    end
  """
  def update(sensor, params \\ %{}, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, params)
    optional_db_update(changes, opts)
  end

  @doc """
  Direct wrapper around ApiServer.Repo.delete, included for consistency.

  """
  def delete(sensor) do
    ApiServer.Repo.delete(sensor)
  end

  @doc """
  Set the last_sync flag. Optional save if the `save: true` flag is set
  """
  def sync(%ApiServer.Sensor{} = sensor, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, %{last_sync_at: DateTime.utc_now()})
    optional_db_update(changes, opts)
  end

  @doc """
  Set the registration field of the sensor to true, optional save if the `save: true` flag
  is set.
  """
  def mark_as_registered(%ApiServer.Sensor{} = sensor, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, %{has_registered: true, last_sync_at: DateTime.utc_now()})
    optional_db_update(changes, opts)
  end

  @doc """
  Set the authenticated/has certificates field for the sensor to true, and optionally save
  """
  def mark_has_certificates(%ApiServer.Sensor{} = sensor, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, %{has_certificates: true})
    optional_db_update(changes, opts)
  end

  @doc """
  Add public key certificate data to the sensor. Optionally save if the `save: true` flag
  is set.
  """
  def add_certificates(%ApiServer.Sensor{} = sensor, pub_key, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, %{public_key: pub_key})
    optional_db_update(changes, opts)
  end

  @doc """
  Mark this sensor as no longer registered. Optionally save if the `save: true` flag
  is set.
  """
  def mark_as_deregistered(%ApiServer.Sensor{} = sensor, opts \\ []) do
    changes = ApiServer.Sensor.changeset(sensor, %{has_registered: false})
    optional_db_update(changes, opts)
  end

  @doc """
  Update this sensor to a new configuration. Optionally save the sensor. To handle
  setting the configuration and retrieving it, you can do:

    case ApiServer.Sensor.update_configuration(%ApiServer.Sensor{}, %{level:"high"}, save: true) do
      {:ok, sensor} ->
        case ApiServer.Sensor.get_configuration_content(sensor) do
          {:ok, configuration} ->
          {:error, reason} ->
        end
    end
  """
  def assign_configuration(%ApiServer.Sensor{} = sensor, %{} = params, opts \\ []) do

    # setup of default values for our configuration params
    config_params = Map.merge(%{level: "default", version: "latest"}, params) |> Map.take([:level, :version])

    # let's make sure our component is actually loaded
    loaded_sensor = sensor |> ApiServer.Repo.preload([:component, :configuration])

    # find our new configuration, given the component of our sensor
    case ApiServer.Component.get_configuration(loaded_sensor.component, config_params) do
      :no_such_configuration ->
        :no_such_configuration
      {:ok, config} ->

        base_changes = ApiServer.Sensor.changeset(loaded_sensor, %{})
        changes = Ecto.Changeset.put_assoc(base_changes, :configuration, config)
        optional_db_update(changes, opts)
    end
  end

  @doc """
  Create the link between a sensor and a component. Optionally save the sensor.
  """
  def assign_component(%ApiServer.Sensor{} = sensor, %ApiServer.Component{} = component, opts \\ []) do

    loaded_sensor = sensor |> ApiServer.Repo.preload(:component)

    # setup the relation, optionally save
#    loaded_sensor.component = component

    base_changes = ApiServer.Sensor.changeset(loaded_sensor, %{})
    changes = Ecto.Changeset.put_assoc(base_changes, :component, component)
    optional_db_update(changes, opts)
  end

  @doc """
  Get the content of the current configuration.
  """
  def get_configuration_content(%ApiServer.Sensor{} = sensor) do
    loaded_sensor = sensor |> ApiServer.Repo.preload(:configuration)

    case loaded_sensor.configuration do
      nil ->
        {:error, "no configuration currently assigned"}
      %ApiServer.Configuration{configuration: config_data} ->
        {:ok, config_data}
    end
  end

  @doc """
  Simple wrapper for counting sensors.
  """
  def count() do
    ApiServer.Repo.aggregate(ApiServer.Sensor, :count, :id)
  end

  @doc """
  Grab sensors older than N minutes.

  """
  def sensors_older_than(minutes) do
    from(s in ApiServer.Sensor, where: s.last_sync_at < ago(^minutes, "minute")) |> ApiServer.Repo.all()
  end

  @doc """
  Method included for terminal state in piped calls. Directly calls ApiServer.Repo.update/1
  """
  def save(%ApiServer.Sensor{} = sensor) do
    ApiServer.Repo.update(sensor)
  end

  # Chain a changeset through our optional save step, and return either
  # the changeset, or the database return.
  defp optional_db_update(changeset, opts) do

    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.update(changeset)
      :false ->
        changeset
    end
  end

  @doc """
  Retrieve a component and all of its configurations, by reference to the component
  name and operating system, which should be unique.
  """
  def get(%{} = params) do

    where_keys = Keyword.take(Map.to_list(params), [:sensor_id, :virtue_id, :public_key, :address, :port])

    comps = ApiServer.Sensor
            |> where(^where_keys)
            |> ApiServer.Repo.all

    case comps do
      [] ->
        :no_matches
      [c] ->
        {:ok, c}
      [h | t] ->
        :multiple_matches
    end
  end

  def clean_json(%ApiServer.Sensor{} = sensor) do
    Map.drop(Map.from_struct(sensor), [:"__meta__", :configuration, :component, :id])
  end
end