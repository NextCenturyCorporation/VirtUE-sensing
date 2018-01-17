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

    # autogen timestamp data
    timestamps()
  end

  def changeset(component, params \\ %{}) do
    component
    |> Ecto.Changeset.cast(params, [:name, :context, :os, :description])
    |> Ecto.Changeset.validate_inclusion(:context, ["virtue", "unikernel", "hypervisor"])
    |> Ecto.Changeset.validate_inclusion(:os, ["linux", "windows", "rump"])
    |> Ecto.Changeset.validate_required([:name])
  end

  @doc """
  Call to create a component. Alias to ApiServer.Component.changeset/2

  Use with a db insert case statement:

      case ApiServer.Repo.insert(ApiServer.Component.create(%{name: "kernel-ps"})) do
        {:ok, component} ->
        {:error, changeset} ->
      end
  """
  def create(params) do
    ApiServer.Component.changeset(%ApiServer.Component{}, params)
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
  Retrieve a specific configration from a component. We load directly
  from the database
  """
  def get_configuration(%ApiServer.Component{} = component, version, opts \\ []) do

    ApiServer.Configuration
    |> ApiServer.Configuration.for_component(component)
    |> where(version: ^version)
    |> ApiServer.Repo.one
  end

  @doc """
  Retrieve a component and all of its configurations, by reference to the component
  name.
  """
  def get_by_name(name) do
    ApiServer.Component
    |> where(name: ^name)
    |> ApiServer.Repo.one
    |> ApiServer.Repo.preload(:configurations)
  end
end