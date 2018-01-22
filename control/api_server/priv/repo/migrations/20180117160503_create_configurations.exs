defmodule ApiServer.Repo.Migrations.CreateConfigurations do
  use Ecto.Migration

  def change do

    create table(:configurations) do
      add :component, :string
      add :version, :string
      add :level, :string
      add :format, :string

      timestamps()
    end

    create index(:configurations, [:component, :version])
    create index(:configurations, [:component, :version, :level], unique: true)
  end
end
