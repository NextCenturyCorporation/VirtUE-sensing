defmodule PKIKey do

  import UUID, only: [uuid4: 0]

  defstruct [:hostname, :port, :challenge, :algo, :size, :csr, :private_key_hash, :public_key]

  @doc """
  Create a new PKI Key struct.

  ### Parameters

    - hostname, port, algo, size
    - hostname, port

  ### Returns

    - %PKIKey{}
  """
  def create(hostname, port, challenge, algo, size, csr, private_key_hash, public_key) do
    %PKIKey{
      hostname: hostname,
      port: port,
      algo: algo,
      size: size,
      challenge: challenge,
      csr: csr,
      private_key_hash: private_key_hash,
      public_key: public_key
    }
  end

  def create(hostname, port, algo, size) do
    %PKIKey{
      hostname: hostname,
      port: port,
      algo: algo,
      size: size
    }
  end

  def create(hostname, port) do
    %PKIKey{
      hostname: hostname,
      port: port,
      algo: Application.get_env(:api_server, :cfssl_default_algo),
      size: Application.get_env(:api_server, :cfssl_default_size)
    }
  end

  @doc """
  Add a private key. The private key will be hashed immediate, and never
  directly stored. The `PKIKey.hash_private_key/1` method will be used to hash
  the private key.

  This is a chainable method.

  ### Parameters

    - %PKIKey{}
    - private key

  ### Returns

   - %PKIKey{}
  """
  def with_private_key(pkikey_struct, private_key) do
    %PKIKey{pkikey_struct| private_key_hash: hash_private_key(private_key)}
  end

  @doc """
  Update the Certificate Signing Request of this request set.


  This is a chainable method.

  ### Parameters

    - %Sensor{}
    - CSR

  ### Returns

   - %PKIKey{}
  """
  def with_csr(pkikey_struct, csr) do
    %PKIKey{pkikey_struct| csr: csr}
  end



  @doc """
  Create a challenge JSON structure that the private key requester needs to host
  and return during the HTTP-01 challenge cycle.

  Our challenge structure looks like:

      {
          "url": "http://host:port/auth/nonce/34/23",
          "token": "random token string",
          "type": "http-savior"
      }

  Where the `url` is the destination and path controlled by the registering entity
  where a JSON structure will be hosted to complete the verification challenge when
  later requesting the signed public key for the current private key.

  The JSON response at the path given in URL will look like:

      {
          "token": ""
      }

  This is a chainable method.

  ### Parameters

    - %PKIKey{}

  ### Returns

   - %PKIKey{}
  """
  def generate_challenge(pkikey_struct) do

    challenge_data = %{
      "url": "http://#{pkikey_struct.hostname}:#{pkikey_struct.port}/path/to/challenge",
      "token": random_token(64),
      "type": "http-savior"
    }
    %PKIKey{pkikey_struct | challenge: Poison.encode!(challenge_data)}
  end

  defp random_token(len) do
    :crypto.strong_rand_bytes(len)
      |> Base.encode64()
      |> binary_part(0, len)
  end

  @doc """
  Update the public key associated with this record.

  This is a chainable method.

  ### Parameters

    - %PKIKey{}
    - public key / string

  ### Returns

   - %PKIKey{}
  """
  def with_public_key(pkikey_struct, pub_key) do
    %PKIKey{pkikey_struct | public_key: pub_key}
  end

  @doc """
  Compare the existing record against a provided PEM encoded private key.


  ### Parameters

    - %PKIKey{}
    - private key - decoded public key PEM data

  ### Returns

   - boolean
  """
  def matches_private_key(pkikey_struct, private_key) do

    case private_key do
      nil ->
        :false
      _ ->
        pkikey_struct.private_key_hash == hash_private_key(private_key)
    end
  end

  @doc """
  Convert an Mnesia record into a %Sensor{} struct.

  ### Parameters

    - Mnesia tuple {PKIKey, id, ...}
    - Mnesia list [id, ...]

  ### Returns

    - %Sensor{}

  """
  def from_mnesia_record(rec) when is_tuple(rec) do
    args = rec |> Tuple.to_list() |> tl |> tl
    apply(PKIKey, :create, args)
  end

  def from_mnesia_record(rec) when is_list(rec) do
    args = rec |> tl
    apply(PKIKey, :create, args)
  end

  @doc """
  Convert a Sensor struct to our Mnesia record format for sensors

  ### Parameters

    - %Sensor{}

  ### Returns

    - mnesia Sensor record compatible tuple
  """
  def to_mnesia_record(
        %PKIKey{
          hostname: hostname,
          port: port,
          algo: algo,
          size: size,
          challenge: challenge,
          csr: csr,
          private_key_hash: private_key_hash,
          public_key: public_key
        },
        %{original: original_record}
      ) do
    mnesia_id = elem(original_record, 1)
    List.to_tuple([PKIKey, mnesia_id, hostname, port, challenge, algo, size, csr, private_key_hash, public_key])
  end

  def to_mnesia_record(
        %PKIKey{
          hostname: hostname,
          port: port,
          algo: algo,
          size: size,
          challenge: challenge,
          csr: csr,
          private_key_hash: private_key_hash,
          public_key: public_key
        }
      ) do
    mnesia_id = uuid4()
    List.to_tuple([PKIKey, mnesia_id, hostname, port, challenge, algo, size, csr, private_key_hash, public_key])
  end

  @doc """
  Hash a private key for storage and comparison. Unless returning the private key to the
  requesting sensor, private keys should always be handled in hashed form.

  ### Parameters

    - string / private key PEM data

  ### Returns

    - string / hashed key in hexidecimal
  """
  def hash_private_key(private_key) do
    Base.encode16(:crypto.hash(:sha256, private_key))
  end
end