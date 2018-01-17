defmodule ApiServer.Repo.Migrations.ConfigurationAddConfigurationData do
  use Ecto.Migration

  def change do

    alter table(:configurations) do
      add :configuration, :string
    end

    create index(:configurations, [:version])
  end
end
