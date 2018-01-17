defmodule ApiServer.Configuration do
  use Ecto.Schema
  import Ecto.Query

  schema "configurations" do

    # Only required fields are the version and related component
    field :version, :string
    belongs_to :component, ApiServer.Component

    # actual configuration blob
    field :configuration, :string, default: "{}"

    # Sane defaults for other fields
    field :level, :string, default: "default"
    field :format, :string, default: "json"
    field :description, :string, default: ""

    # auto gen timestamp fields
    timestamps()
  end

  def for_component(query, component) do
     from c in query,
      join: p in assoc(c, :component),
      where: p.id == ^component.id
  end


  def changeset(person, params \\ %{}) do
    person
    |> Ecto.Changeset.cast(params, [:component_id, :version, :level, :format, :description, :configuration])
    |> Ecto.Changeset.validate_inclusion(:level, ["off", "default", "low", "high", "adversarial"])
    |> Ecto.Changeset.validate_required([:component_id, :version])
  end

  def create(%ApiServer.Component{id: component_id}, version) do
    ApiServer.Configuration.changeset(%ApiServer.Configuration{}, %{component_id: component_id, version: version})
  end

end