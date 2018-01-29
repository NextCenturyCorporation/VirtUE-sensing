defmodule ApiServer.Repo.Migrations.AuthchallengeAddRelationSensor do
  use Ecto.Migration

  def change do

    alter table(:authchallenges) do
      add :sensor_id, references(:sensors)
    end
  end
end
