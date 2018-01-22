defmodule ApiServer.Repo.Migrations.UpdateConfiguration do
  use Ecto.Migration

  def change do

    alter table(:configurations) do
      remove :component
      add :description, :string
      add :component_id, references(:components)
    end

    drop_if_exists index(:configurations, [:component, :version])
    drop_if_exists index(:configurations, [:component, :version, :level])

    create index(:configurations, [:component_id])
  end
end
