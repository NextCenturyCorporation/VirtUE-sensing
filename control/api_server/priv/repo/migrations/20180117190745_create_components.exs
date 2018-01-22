defmodule ApiServer.Repo.Migrations.CreateComponents do
  use Ecto.Migration

  def change do

    create table(:components) do
      add :name, :string
      add :context, :string
      add :os, :string
      add :description, :string

      timestamps()
    end

    create index(:components, [:name])
    create index(:components, [:name, :context])

  end

end
