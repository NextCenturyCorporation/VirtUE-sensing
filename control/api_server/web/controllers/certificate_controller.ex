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

    IO.puts("  % Received request for signed public key - initiating verification")

    # lookup our PKIKey record based on csr
    case ApiServer.DatabaseUtils.record_for_csr(csr) do

      # ok - it's a CSR we've seen before, and we don't yet have a public key
      {:ok, %PKIKey{challenge: challenge_raw, hostname: hostname, port: port, public_key: nil}=pkikey} ->

        IO.puts("  < issuing verification challenge for hostname(#{hostname}) port(#{port})")
        # issue a challenge request
        case verify_http_01_challenge(challenge_raw) do

          :verified ->
            IO.puts("  & requester verified, contacting CFSSL for signed certificate")

            # format our data for the CFSSL signing request. This gets really cumbersome, as
            # we end up creating multiple base64 encodings of things, in exact combinations, along
            # with an HMAC of a certain subset of data.
            signing_request_data = %{
              "hostname": hostname,
              "hosts": [hostname],
              "certificate_request": csr,
              "profile": "default",
              "label": "client auth",
              "bundle": :false
            }

            # serialized signing request
            signing_request_string = Poison.encode!(signing_request_data)

            # shared secret token based hmac of the request
            IO.puts("  # using CFSSL_SHARED_SECRET(#{ApiServer.ConfigurationUtils.get_cfssl_shared_secret()})")
            {:ok, decoded_cfssl_secret} = Base.decode16(ApiServer.ConfigurationUtils.get_cfssl_shared_secret(), case: :mixed)
            token_hmac = :crypto.hmac(:sha256, decoded_cfssl_secret, signing_request_string)
            token = token_hmac |> Base.encode64()
            signing_request = signing_request_string |> Base.encode64()

            signing_payload = Poison.encode!(%{
              "request": signing_request,
              "token": token
            })
            IO.puts("  # signing_payload(#{signing_payload})")

            # issue the request to CFSSL
            cfssl_host = Application.get_env(:api_server, :cfssl_host)
            cfssl_port = Application.get_env(:api_server, :cfssl_port)
            case HTTPoison.post("http://#{cfssl_host}:#{cfssl_port}/api/v1/cfssl/authsign", signing_payload, [], [timeout: 10_000, recv_timeout: 10_000, connect_timeout: 10_000]) do

              # Possible success, there could still be something in the response structure that signals
              # an error
              {:ok, %HTTPoison.Response{status_code: 200, body: body}} ->

                # pull apart the JSON and let's take a look
                case Poison.decode(body) do

                  # JSON structure is parsable, let's see if it was successful
                  {:ok, public_key_data} ->

                    case public_key_data["success"] do

                      # Success! Now we can record the public key and return it to the requester
                      :true ->
                        :ok

                        # record the public key in our data structure
                        case ApiServer.DatabaseUtils.update_pkikey(PKIKey.with_public_key(pkikey, public_key_data["result"]["certificate"])) do

                          # return the public key to the requester
                          :ok ->
                            conn
                              |> put_status(200)
                              |> json(
                                  %{
                                    error: :false,
                                    timestamp: DateTime.to_string(DateTime.utc_now()),
                                    certificate: public_key_data["result"]["certificate"]
                                  }
                                 )

                          {:error, :no_such_record} ->
                            IO.puts("  ! Couldn't record the updated public key data - no record found")
                            conn |> req_error(500, "There was a problem recording the public key to API storage, aborting")

                        end

                      # Something wrong with the request, let's log it, and return 500
                      :false ->
                        IO.puts("  ! The request was not successful")
                        Enum.each(public_key_data["errors"], fn (m) -> IO.puts("    - #{m}") end)

                        conn |> req_error(500, public_key_data["errors"])

                    end

                  # Something was wrong with the JSON, let's bail out
                  {:error, err} ->
                    IO.puts("  ! Error parsing JSON response from CFSSL: #{err}")
                    conn |> req_error(500, "There was an error parsing the JSON response from CFSSL")

                end

              # we got HTTP/?, and something was wrong
              {:ok, %HTTPoison.Response{status_code: status_code}} ->
                IO.puts("  ! CFSSL returned status_code(#{status_code}) in response to signed public key request")
                conn |> req_error(500, "Got status_code(#{status_code}) from CFSSL when requesting a signed public key")

            end

          {:unverified, reason} ->
            IO.puts("  - HTTP-01-SAVIOR challenge failed, requester not verified: #{reason}")
            conn |> req_error(401, reason)

        end

      # weird - someone/thing is trying to get a new/duplicate public key using a CSR that's already been
      # issued a signed public certificate.
      {:ok, %PKIKey{hostname: hostname, port: port, public_key: pubkey}} ->
        IO.puts("  ! CSR for sensor hostname(#{hostname}) port(#{port}) was used to try to reissue a signed public key")
        conn |> req_error(400, "Cannot generate a new public key for a CSR that has already been issued a signed public key")

      # no record of this CSR in the database
      {:error, :no_such_pkikey} ->
        IO.puts("  - CSR does not match any recorded private key requests")
        conn |> req_error(400, "CSR does not correspond to any recorded key requests")

    end

  end

  def public_key_signed(conn, _) do
    conn |> req_error(400, "Missing certificate_request parameter for public key generation and signing")
  end

  defp req_error(conn, code, msg) when is_binary(msg) do
    conn |> req_error(code, [msg])
  end

  defp req_error(conn, code, msg) when is_list(msg) do
    conn
    |> put_status(code)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           messages: [msg]
         }
       )
  end

  @doc """
  Issue a request to a host and attempt to verify the challenge data as part of
  the certificate generation for signed public keys.

  This is the HTTP-01-SAVIOR challenge. The Challenge data is generated by the
  PKIKey struct as part of the initial private key request cycle.

  ### Parameters

    - challenge: non-decoded JSON blob of challenge data

  ### Side Effects

    - HTTP request issued to sensor

  ### Returns

    :verified

    {:unverified, reason}

  """
  def verify_http_01_challenge(challenge) do

    %{"url" => challenge_url, "token" => challenge_token, "type" => _} = Poison.decode!(challenge)

    # issue the request
    case HTTPoison.get(challenge_url, [], [timeout: 5000, recv_timeout: 5000, connect_timeout: 5000]) do

      # HTTP/200 - we'll need to parse the results
      {:ok, %HTTPoison.Response{status_code: 200, body: json_body}} ->

        case Poison.decode(json_body) do

          # challenge response token equals our existing token
          {:ok, %{"token" => ^challenge_token}} ->
            :verified

          # token mismatch
          {:ok, _} ->
            {:unverified, "Requester's challenge response token doesn't match the recorded token"}

          {:error, _} ->
            {:unverified, "Encountered an error parsing the JSON response from the Requester's HTTP service."}
        end

      # HTTP/? - something went wrong on the other end
      {:ok, %HTTPoison.Response{status_code: status_code}} ->
        {:unverified, "Requester's HTTP service returned status_code(#{status_code})."}

      # Socket/Control/Request error
      {:error, %HTTPoison.Error{reason: reason}} ->
        {:unverified, reason}
    end

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