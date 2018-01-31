defmodule ApiServer.Repo.Migrations.SensorIdUnique do
  use Ecto.Migration

  def change do

    drop_if_exists index(:sensors, [:sensor_id])
    create index(:sensors, [:sensor_id], unique: true)
    drop_if_exists index(:sensors, [:public_key])
    create index(:sensors, [:public_key], unique: true)
  end
end
