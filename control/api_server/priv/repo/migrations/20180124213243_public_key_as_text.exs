defmodule ApiServer.Repo.Migrations.PublicKeyAsText do
  use Ecto.Migration

  def change do

    drop_if_exists index(:sensors, [:public_key])

    alter table(:sensors) do
      modify :public_key, :text
    end

    create index(:sensors, [:public_key], unique: true)
  end
end
