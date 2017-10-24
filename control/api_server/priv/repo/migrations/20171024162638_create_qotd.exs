defmodule ApiServer.Repo.Migrations.CreateQotd do
  use Ecto.Migration

  def change do
    create table(:qotd) do
      add :msg, :string

      timestamps
    end

    create unique_index(:qotd, [:msg])
  end
end
