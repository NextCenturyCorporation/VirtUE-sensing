defmodule ApiServer.Repo.Migrations.AuthchallengeRefDelete do
  use Ecto.Migration

  def change do

    drop constraint(:authchallenges, "authchallenges_sensor_id_fkey")
    alter table(:authchallenges) do
      modify :sensor_id, references(:sensors, on_delete: :delete_all)
    end

  end
end
