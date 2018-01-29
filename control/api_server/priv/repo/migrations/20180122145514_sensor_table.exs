defmodule ApiServer.Repo.Migrations.SensorTable do
  use Ecto.Migration

  def change do

    create table (:sensors) do

      # basic sensor information, just like we once stored in ETS
      add :sensor_id, :string
      add :virtue_id, :string
      add :username, :string
      add :address, :string
      add :port, :integer
      add :public_key, :string
      add :kafka_topic, :string

      # relation to components
      add :component_id, references(:components)

      # relation to configs
      add :configuration_id, references(:configurations)

      # most recent synchronization
      add :last_sync_at, :utc_datetime

      # did we go through the authentication cycle
      add :has_certificates, :boolean, default: false

      # did we go through the registration cycle
      add :has_registered, :boolean, default: false

      timestamps()
    end

    create index(:sensors, [:sensor_id])
    create index(:sensors, [:virtue_id])
    create index(:sensors, [:public_key])
    create index(:sensors, [:username])
  end
end
