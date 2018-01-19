defmodule ApiServer.Configuration do
  @moduledoc """

  See ApiServer.Component for more on the convenience methods for working with Components
  and configurations.


  Get a specific component version:

      ApiServer.Configuration.for_component("kernel-ps")
      |> Ecto.Query.where(level: "default")
      |> ApiServer.Repo.one

  Use a component object to retrieve a specific version:

      ApiServer.Configuration
      |> ApiServer.Configuration.for_component(component)
      |> Ecto.Query.where(level: "default")
      |> ApiServer.Repo.one

  """

  use Ecto.Schema
  import Ecto.Query

  schema "configurations" do

    # Only required fields are the version and related component
    field :level, :string, default: "default"
    belongs_to :component, ApiServer.Component

    # actual configuration blob
    field :configuration, :string, default: "{}"

    # Sane defaults for other fields
    field :version, :string, default: "latest"
    field :format, :string, default: "json"
    field :description, :string, default: ""

    # auto gen timestamp fields
    timestamps()
  end

  @doc """
  Find configurations linked to a particular component struct as part of a larger
  or compound query:

    ApiServer.Configuration |> ApiServer.Configuration.for_component(component)
  """
  def for_component(query, %ApiServer.Component{} = component) do
     from c in query,
      join: p in assoc(c, :component),
      where: p.id == ^component.id
  end

  @doc """
  Retrieve configurations linked to a particular component using the component name

    ApiServer.Configuration.for_component("kernel-ps")
  """
  def for_component(component_name) do
    component = ApiServer.Component.get_components(%{name: component_name})

    ApiServer.Configuration
    |> ApiServer.Configuration.for_component(component)
  end

  @doc """
  Update a configuration, optionally saving it back to the database:

    case ApiServer.Configuration.update_configuration(conf, %{level: "..."}, save: true) do
      {:ok, configuration} ->
      {:error, changeset} ->
    end
  """
  def update_configuration(configuration, params, opts \\ []) do
    changes = ApiServer.Configuration.changeset(configuration, params)

    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.update(changes)
      :false ->
        changes
    end
  end

  def changeset(person, params \\ %{}) do
    person
    |> Ecto.Changeset.cast(params, [:component_id, :version, :level, :format, :description, :configuration])
    |> Ecto.Changeset.validate_inclusion(:level, ["off", "default", "low", "high", "adversarial"])
    |> Ecto.Changeset.validate_required([:component_id, :level])
  end

  def create(%ApiServer.Component{id: component_id}, level) do
    ApiServer.Configuration.changeset(%ApiServer.Configuration{}, %{component_id: component_id, level: level})
  end

end