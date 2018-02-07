defmodule ApiServer.AuthenticationUtils do
  @moduledoc """
  Tools and methods for working with authentication information.
  """

  @doc """
  given the decoded certificate on the connection (the client certificate) and a provided
  public key, verify that they are one and the same
  """
  def matches_client_certificate(%Plug.Conn{private: %{client_certificate: certificate}}, pub_key) do

    IO.puts("  == verifying client certificate and provided public key match")

    # convert the pubkey (in PEM form) into a DER encoding
    [{_, der_cert, _}] = :public_key.pem_decode(pub_key)

    # convert the client certificate into the proper encoding
    client_der_cert = :public_key.pkix_encode(:"OTPCertificate", certificate, :otp)

    # are they the same?
    case client_der_cert == der_cert do
      true ->
        IO.puts("    == True")
        true
      false ->
        IO.puts("    == False")
        false
    end
  end


  @doc """
  Compare the decoded certificate from a server connection with a public key
  """
  def matches_server_certificate({:OTPCertificate, certificate}, public_key) do

    IO.puts("  == verifying server certificate and cached public key match")

    # conver the pub key to DER encoding
    [{_, der_cert, _}] = :public_key.pem_decode(public_key)

    # convert the server certificate to the proper encoding
    server_der_cert = :public_key.pkix_encode(:"OTPCertificate", certificate, :otp)

    # are they the same?
    der_cert == server_der_cert
  end

  @doc """
  Callback function for handling verification of TLS certificates during the
  request cycle. Included as `ssl` options for an HTTPoison request like:

    [ssl: [{:verify_fun, {&ApiServer.RegistrationController.pin_verify/3, {:pin, "PINNED PUBLIC KEY IN PEM}}}]
  """
  def pin_verify(_, {:extension, _}, state) do
    IO.puts("pin_verify/extension called - passing state")
    {:unknown, state}
  end

  def pin_verify(cert, _, state) do

    case state do
      {:pin, pin_cert} ->
        IO.puts("pin_verify called")

        case ApiServer.AuthenticationUtils.matches_server_certificate({:OTPCertificate, cert}, pin_cert) do
          true ->
            IO.puts("  + pinned sensor certificate validated")
            {:valid, "OK"}
          false ->
            IO.puts("  - pinned sensor certificate failed validation")
            {:fail, :fingerprint_no_match}
        end
      _ ->
        IO.puts("Extraneous call to pin_verify with state")
        IO.puts(state)
        {:unknown, state}
    end

  end
end