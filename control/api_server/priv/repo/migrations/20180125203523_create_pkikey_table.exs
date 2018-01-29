defmodule ApiServer.Repo.Migrations.CreatePkikeyTable do
  use Ecto.Migration

  def change do

    create table(:authchallenges) do

      add :hostname, :string
      add :port, :integer
      add :algo, :string
      add :size, :integer
      add :challenge, :text
      add :csr, :text
      add :private_key_hash, :text
      add :public_key, :text

      timestamps()
    end

    create index(:authchallenges, [:private_key_hash], unique: true)
    create index(:authchallenges, [:csr], unique: true)
    create index(:authchallenges, [:public_key], unique: true)
  end
end
