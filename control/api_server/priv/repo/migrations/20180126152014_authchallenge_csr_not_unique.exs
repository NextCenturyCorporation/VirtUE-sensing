defmodule ApiServer.Repo.Migrations.AuthchallengeCsrNotUnique do
  use Ecto.Migration

  def change do

    drop_if_exists index(:authchallenges, [:csr], unique: true)
    create index(:authchallenges, [:csr])
  end
end
