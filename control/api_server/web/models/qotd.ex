defmodule ApiServer.Qotd do
  use ApiServer.Web, :model

  schema "qotd" do
    field :msg, :string

    timestamps
  end
end