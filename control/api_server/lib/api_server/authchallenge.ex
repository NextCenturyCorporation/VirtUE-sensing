defmodule ApiServer.AuthChallenge do
  @moduledoc """
  Schema for working with authentication challenges.
  """

  use Ecto.Schema
  import Ecto.Query

  schema "authchallenges" do

    field :hostname, :string
    field :port, :integer

    # These defaults are applied at compile time
    field :algo, :string, default: Application.get_env(:api_server, :cfssl_default_algo)
    field :size, :integer, default: Application.get_env(:api_server, :cfssl_default_size)

    # text records
    field :challenge, :string
    field :csr, :string
    field :private_key_hash, :string
    field :public_key, :string

    # relationship to a sensor
    belongs_to :sensor, ApiServer.Sensor

    # basic time stamping
    timestamps()
  end

  @doc """
  Standard changeset casting and validation.
  """
  def changeset(challenge, params \\ %{}) do

    challenge
    |> Ecto.Changeset.cast(params, [:hostname, :port, :algo, :size, :challenge, :csr, :private_key_hash, :public_key, :sensor_id])
    |> Ecto.Changeset.validate_required([:hostname, :port, :algo, :size])
    |> Ecto.Changeset.validate_number(:port, greater_than: 0)
    |> Ecto.Changeset.validate_number(:port, less_than: 65536)

  end

  @doc """
  Create a new authentication challenge, optionally saving directly to the DB.
  """
  def create(params, opts \\ []) do
    IO.puts("starting create")
    changes = ApiServer.AuthChallenge.changeset(%ApiServer.AuthChallenge{}, params)
                |> opt_create_challenge(params, opts)

    IO.puts("checking :save")

    case Keyword.get(opts, :save, false) do
      :true ->
        IO.puts("saving")
        ApiServer.Repo.insert(changes)
      :false ->
        changes
    end

  end

  # make the option to generate a challenge at create time pipe-able
  defp opt_create_challenge(changeset, %{} = params, opts) do
    IO.puts("generating challenge data")
    case Keyword.get(opts, :create_challenge, false) do
      false ->
        changeset
      true ->
        changeset |> Ecto.Changeset.put_change(:challenge, Poison.encode!(generate_challenge_data(params)))
    end
  end

  @doc """
  Update an existing Authentication Challenge object, optionally directly saving.
  """
  def update(%ApiServer.AuthChallenge{} = challenge, %{} = params, opts \\ []) do
    changes = ApiServer.AuthChallenge.changeset(challenge, params)
    optional_db_update(changes, opts)
  end

  @doc """
  Basic get method that returns error atoms when not exactly one record is retrieved.
  """
  def get(%{} = params) do
    where_keys = Keyword.take(Map.to_list(params), [:hostname, :port, :algo, :size, :challenge, :csr, :private_key_hash, :public_key])

    comps = ApiServer.AuthChallenge
            |> where(^where_keys)
            |> ApiServer.Repo.all

    case comps do
      [] ->
        :no_matches
      [c] ->
        {:ok, c}
      [h | t] ->
        :multiple_matches
    end
  end

  # Chain a changeset through our optional save step, and return either
  # the changeset, or the database return.
  defp optional_db_update(changeset, opts) do
    case Keyword.get(opts, :save, false) do
      :true ->
        ApiServer.Repo.update(changeset)
      :false ->
        changeset
    end
  end

  # Generate the encoded challenge data for an authentication callback
  defp generate_challenge_data(%{hostname: hostname, port: port}) do
    challenge_data = %{
      "url": "http://#{hostname}:#{port}/path/to/challenge",
      "token": random_token(64),
      "type": "http-savior"
    }
  end

  # create a cryptographically random token of the given length
  defp random_token(len) when is_integer(len) do
    :crypto.strong_rand_bytes(len)
    |> Base.encode64()
    |> binary_part(0, len)
  end


end