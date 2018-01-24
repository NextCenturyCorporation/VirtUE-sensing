defmodule ApiServer.Component do
  @moduledoc """


  # get a component
  component = ApiServer.Component.get_by_name("kernel-ps")

  # Add a config version
  component |> ApiServer.Component.add_configuration(%{version: "temp"}, save: true)

  # get a config version
  config = ApiServer.Component.get_configuration(component, "temp")

  # update a config
  config |> ApiServer.Configuration.update_configuration(%{version: "2018-01-20T10:10:10"}, save: true)



  Create a component:

      case ApiServer.Repo.insert(ApiServer.Component.create("kernel-ps")) do
        {:ok, component} ->
        {:error, changeset} ->
      end
  """
  use Ecto.Schema
  import Ecto.Query

  schema "components" do

    # only required field is the name
    field :name, :string

    # generally sane defaults for everything else
    field :context, :string, default: "virtue"
    field :os, :string, default: "linux"
    field :description, :string, default: ""

    # related configurations
    has_many :configurations, ApiServer.Configuration

    # related sensors
    has_many :sensors, ApiServer.Sensor

    # autogen timestamp data
    timestamps()
  end

  def changeset(component, params \\ %{}) do
    component
    |> Ecto.Changeset.cast(params, [:name, :context, :os, :description])
    |> Ecto.Changeset.validate_inclusion(:context, ["virtue", "unikernel", "hypervisor"])
    |> Ecto.Changeset.validate_inclusion(:os, ["linux", "windows", "rump"])
    |> Ecto.Changeset.validate_required([:name, :context, :os])
  end

  @doc """
  Call to create a component. Alias to ApiServer.Component.changeset/2

  Use with a db insert case statement:

      case ApiServer.Repo.insert(ApiServer.Component.create(%{name: "kernel-ps"})) do
        {:ok, component} ->
        {:error, changeset} ->
      end

  Optionally call with save: true -

    case ApiServer.Component.create(%{name: "kernel-ps"}, save: true) do
      {:ok, component}
      {:error, changeset}
    end
  """
  def create(params, opts \\ []) do

    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.insert(ApiServer.Component.changeset(%ApiServer.Component{}, params))
      :false ->
        ApiServer.Component.changeset(%ApiServer.Component{}, params)
    end
  end

  @doc """
  Add a configuration to an existing component. Call like:

      component = ...
      case ApiServer.Repo.insert(ApiServer.Component.add_configuration(component, %{version: "2018-01-18"})) do
        {:ok, configuration} ->
        {:error, changeset} ->
      end

  Or use the safe flag

      component = ...
      case ApiServer.Component.add_configuration(component, %{version: "2018-01-19"}, save: true) do
        {:ok, configuration} ->
        {:error, changeset} ->
      end
  """
  def add_configuration(component, params, opts \\ []) do
    config = component
    |> Ecto.build_assoc(:configurations)
    |> ApiServer.Configuration.changeset(params)

    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.insert(config)
      :false ->
        config
    end
  end

  @doc """
  Retrieve a specific configration from a component. We load directly from the database. Selectors
  are used to determine how individual configurations are selected.

  When retrieving a configuration, the following defaults are used:

    - level: default
    - version: latest

  # Selectors

   - version: find the specific version of a configuration for the component
   - level: find the most recent configuration at the specific observation level

  #
  """
  def get_configuration(component, params \\ %{}, opts \\ [])
  def get_configuration(%ApiServer.Component{} = component, %{} = params, opts) do

    # setup our keyworld list, with defaults
    where_keys = Keyword.take(Keyword.merge([level: "default", version: "latest"], Map.to_list(params)), [:version, :level])

    # query away
    conf = ApiServer.Configuration
    |> ApiServer.Configuration.for_component(component)
    |> where(^where_keys)
    |> ApiServer.Repo.one
    |> ApiServer.Repo.preload(:component)

    case conf do
      nil ->
        :no_such_configuration
      c ->
        {:ok, c}
    end
  end

  def get_configuration(%{} = component, %{} = params, opts) do

    case ApiServer.Component.get_component(component) do
      :no_matches ->
        :no_component_matches
      :multiple_matches ->
        :multiple_component_matches
      {:ok, comp} ->
        ApiServer.Component.get_configuration(comp, params, opts)
    end
  end


  @doc """
  Retrieve all of the available configurations for the given component.
  """
  def get_configurations(%ApiServer.Component{} = component, opts \\ []) do
    ApiServer.Configuration
    |> ApiServer.Configuration.for_component(component)
    |> ApiServer.Repo.all
  end


  @doc """
  Retrieve all components matching the given selectors.

  # Selectors

  One or more of:

    - name
    - os (linux, windows, rump)
    - context (virtue, unikernel, hypervisor)
  """
  def get_components(%{} = params) do
    key_list = Map.to_list(params)
    ApiServer.Component
    |> where(^key_list)
    |> ApiServer.Repo.all
  end

  @doc """
  Retrieve a component and all of its configurations, by reference to the component
  name and operating system, which should be unique.
  """
  def get_component(%{} = params) do

    where_keys = Keyword.take(Map.to_list(params), [:os, :context, :name])

    comps = ApiServer.Component
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

  @doc """
  Verify if the sensor with the given parameters has a default configuration.
  """
  def has_default_configuration?(%{name: sensor_name, os: sensor_os, context: sensor_context})
      when is_binary(sensor_name)
    and is_binary(sensor_os)
    and is_binary(sensor_context) do

    case ApiServer.Component.get_configuration(%{os: sensor_os, name: sensor_name, context: sensor_context}) do
      :no_component_matches ->
        false
      :multiple_component_matches ->
        false
      :no_such_configuration ->
        false
      {:ok, config} ->
        true
    end
  end

  def update_component(component, params \\ %{}, opts \\ []) do
    changes = ApiServer.Component.changeset(component, params)

    case Keyword.get(opts, :save, :false) do
      :false ->
        changes
      :true ->
        ApiServer.Repo.update(changes)
    end
  end
end