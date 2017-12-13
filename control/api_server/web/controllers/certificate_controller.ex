defmodule ApiServer.CertificateController do
  @moduledoc """
  Manage client and server certificates.

  The Sensing API uses an HTTP-01 challenge scheme to validate host
  control when issuing certificates.
  """

  use ApiServer.Web, :controller

  import ApiServer.ExtractionPlug, only: [is_hostname: 1, is_sensor_port: 1]

  @doc """
  Create a new TLS private key and Certificate Signing Request, returning
  both to the requester, along with verification challenge criteria.

  ### Validations:

  ### Available:

  ### Side-effects:

  ### Return:

  """
  def private_key_new(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "hostname" => hostname,
            "port" => port,
            "key_algo" => algo,
            "key_size" => size
          }
        } = conn, _) do

    case issue_private_key_request(hostname, port, algo, size) do
      {:ok, priv_key, csr, challenge} ->
        conn
        |> put_status(200)
        |> json(
             %{
               error: :false,
               timestamp: DateTime.to_string(DateTime.utc_now()),
               private_key: priv_key,
               certificate_request: csr,
               challenge: Poison.decode!(challenge)
             }
           )
      {:error, messages} ->
        conn
        |> put_status(500)
        |> json(
             %{
               error: :true,
               timestamp: DateTime.to_string(DateTime.utc_now()),
               messages: messages
             }
           )
    end
  end

  def private_key_new(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "hostname" => hostname,
            "port" => port
          }
        } = conn, _) do

    case issue_private_key_request(hostname, port, Application.get_env(:api_server, :cfssl_default_algo), Application.get_env(:api_server, :cfssl_default_size)) do
      {:ok, priv_key, csr, challenge} ->
        conn
          |> put_status(200)
          |> json(
              %{
                error: :false,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                private_key: priv_key,
                certificate_request: csr,
                challenge: Poison.decode!(challenge)
              }
             )
      {:error, messages} ->
        conn
          |> put_status(500)
          |> json(
              %{
                error: :true,
                timestamp: DateTime.to_string(DateTime.utc_now()),
                messages: messages
              }
             )
    end
  end

  def private_key_new(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "Missing hostname parameter for private key and certificate request generation"
         }
       )
  end

  # Call out to CFSSL to create a private key and certificate signing
  # request. Most of the request data is generated in situ from configuration
  # data within the Sensing API.
  #
  # Returns:
  #
  #   {:error, [messages]}
  #
  #   {:ok, private key, csr, encoded JSON nonce for HTTP-01 challenge}
  defp issue_private_key_request(hostname, port, algo, size) do
    IO.puts("  & issuing private key request to CFSSL for hostname(#{hostname}:#{port}), algo(#{algo}), size(#{size})")

    # we're going to check a handful of failure conditions up front to validate our parameters, only
    # then issuing a full request
    cond do

      ! is_hostname(hostname) ->
        {:error, ["Invalid hostname(#{hostname})"]}

      ! is_sensor_port(port) ->
        {:error, ["Invalid port(#{port})"]}

      :true ->
        # setup our request record
        priv_key_request = Poison.encode!(%{
          "key" => %{
            "algo" => algo,
            "size" => size
          },
          "hosts" => [hostname],
          "names" => [
            %{
              "C" => "US",
              "ST" => "Virginia",
              "L" => "Arlington",
              "O" => hostname
            }
          ],
          "CN" => hostname
        })

        # let's POST it to CFSSL
        cfssl_host = Application.get_env(:api_server, :cfssl_host)
        cfssl_port = Application.get_env(:api_server, :cfssl_port)
        case HTTPoison.post("http://#{cfssl_host}:#{cfssl_port}/api/v1/cfssl/newkey", priv_key_request, [], [timeout: 10_000, recv_timeout: 10_000, connect_timeout: 10_000]) do

          # Possible success, there could still be something in the response structure that signals
          # an error
          {:ok, %HTTPoison.Response{status_code: 200, body: body}} ->
            IO.puts("  = HTTP/200 - parsing CFSSL response")

            case Poison.decode(body) do

              {:ok, priv_key_data} ->
                case priv_key_data["success"] do

                  # We had a successful key request, let's pull it apart, store a record, and respond
                  :true ->

                    # let's build up a PKIKey struct
                    key = PKIKey.create(hostname, port, algo, size)
                      |> PKIKey.with_private_key(priv_key_data["result"]["private_key"])
                      |> PKIKey.with_csr(priv_key_data["result"]["certificate_request"])
                      |> PKIKey.generate_challenge()

                    # let's record this to the database
                    case ApiServer.DatabaseUtils.record_private_key_request(key) do

                      # now we can return the data for the requester
                      {:ok, _} ->
                        {:ok, priv_key_data["result"]["private_key"], priv_key_data["result"]["certificate_request"], key.challenge}

                      # for some reason we had a problem recording this to the database...
                      {:error, reason} ->
                        {:error, [reason]}
                    end


                  # something was wrong with our request (could be algorithm choice, size, naming info, etc
                  :false ->
                    {:error, priv_key_data["errors"] ++ priv_key_data["messages"]}
                end
              {:error, _} ->
                {:error, ["Invalid JSON received from CFSSL", body]}
            end

          # We got a non- HTTP/200 response
          {:ok, %HTTPoison.Response{status_code: status_code}} ->
            IO.puts("Got status_code(#{status_code}) from CFSSL when requesting a new private key and CSR")
            {:error, ["Got status_code(#{status_code}) from CFSSL when requesting a new private key and CSR"]}

        end
    end
  end

  @doc """
  Request a signed public key to compliment an already retrieved private key
  and CSR.

  ### Validations:

  ### Available:

  ### Side-effects:

  ### Return:

  """
  def public_key_signed(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "certificate_request" => csr
          }
        } = conn, _) do

  end

  def public_key_signed(conn, _) do
    conn
      |> put_status(400)
      |> json(
            %{
              error: :true,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              msg: "Missing certificate_request parameter for public key generation and signing"
            }
         )
  end

  @doc """
  Revoke a certificate based on a private key.

  ### Validations:

  ### Available:

  ### Side-effects:

  ### Return:

  """
  def revoke_key(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "certificate_request" => csr
          }
        }
      ) do

  end

  def revoke_key(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "Missing certificate_request parameter for key revocation"
         }
       )
  end

end