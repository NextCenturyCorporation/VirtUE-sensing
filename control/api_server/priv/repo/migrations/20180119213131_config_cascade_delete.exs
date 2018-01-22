defmodule ApiServer.Repo.Migrations.ConfigCascadeDelete do
  use Ecto.Migration

  def change do

    drop constraint(:configurations, "configurations_component_id_fkey")
    alter table(:configurations) do
      modify :component_id, references(:components, on_delete: :delete_all)
    end
  end
end
