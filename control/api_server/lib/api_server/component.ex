defmodule ApiServer.Component do
  use Ecto.Schema

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

  def create(component) do
    ApiServer.Component.changeset(%ApiServer.Component{}, %{name: component})
  end
end