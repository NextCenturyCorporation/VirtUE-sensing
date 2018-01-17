defmodule ApiServer.Configuration do
  use Ecto.Schema

  schema "configurations" do
    field :component, :string
    field :version, :string
    field :level, :string, default: "default"
    field :format, :string, default: "json"

    timestamps()
  end


  def changeset(person, params \\ %{}) do
    person
    |> Ecto.Changeset.cast(params, [:component, :version, :level, :format])
    |> Ecto.Changeset.validate_inclusion(:level, ["off", "default", "low", "high", "adversarial"])
    |> Ecto.Changeset.validate_required([:component, :version])
  end

  def create(component, version) do
    ApiServer.Configuration.changeset(%ApiServer.Configuration{}, %{component: component, version: version})
  end

end