defmodule ApiServer.Plugs.Authenticate do
  import Plug.Conn
  import Phoenix.Controller
  require Logger

  def init(default) do
    default
  end

  def call(%Plug.Conn{private: %{client_certificate_common_name: tls_common_name, client_certificate: certificate}} = conn, _opts) do

    ## Verification process
    # 1. hostname verification (not yet done)
    # 2. date bounding verification
    # 3. CRL checking (not yet done)
    # 4. not-self-signed
    # 5. certificate chain validation with Savior CA
    # 6. algorithmic complexity of signature algo (not yet done)
    IO.puts("% authenticating request")

    # extract all of the bits and pieces of the certificate
    # See:
    #   http://erlang.org/doc/apps/public_key/using_public_key.html#id67373
    {
      :"OTPCertificate",
      {
        :"OTPTBSCertificate", :v3, cert_serial_number,
        {:"SignatureAlgorithm", _, _},
        {:rdnSequence, _}, # issuer
        {:"Validity",
          cert_not_before,
          cert_not_after
        },
        {:rdnSequence, _}, # subject/CN/etc
        {:"OTPSubjectPublicKeyInfo",
          {:"PublicKeyAlgorithm", _, _},
          {:"RSAPublicKey", cert_modulus, cert_exponent}
        },
        :asn1_NOVALUE,
        :asn1_NOVALUE,
        _, # extensions
      },
      {:"SignatureAlgorithm", _, _},
      cert_signature
    } = certificate

    # date bounds checks
    # According to IETF RFC5280 these time will be formatted as:
    #
    #   UTCTime(for datetimes through year 2049)
    #   GeneralizedTime(for datetimes starting in year 2050)
    #
    # The UTCTime is based on an ASN.1 time format which looks
    # roughly like:
    #
    #   YYMMDDHHMMSSZ
    #
    # Where if YY is >= 50, the year is 19YY and if YY < 50
    # the year is 20YY. Sigh.
    #
    # Can't find a decent parser for these, which is a pain.
    IO.puts("  ? calculating datetime boundaries")
    now_dt = Timex.now()

    IO.puts("    @ not_before(#{elem(cert_not_before, 1)})")
    not_before_dt = cert_time_to_date_time(cert_not_before)

    IO.puts("    @ not_after(#{elem(cert_not_after, 1)})")
    not_after_dt = cert_time_to_date_time(cert_not_after)

    # we need to grab the CA certificate we'll use for validation
    {:ok, ca_root_cert} = ApiServer.ConfigurationUtils.load_ca_certificate()

    # let's evaluate everything we've pulled together so far
    # and decide what to do
    cond do

      Timex.before?(now_dt, not_before_dt) ->
        reject_connection(conn, 495, "Certificate isn't yet valid (not_before)")

      Timex.after?(now_dt, not_after_dt) ->
        reject_connection(conn, 495, "Certificate is not longer valid (not_after)")

      :public_key.pkix_is_self_signed(certificate) ->
        reject_connection(conn, 495, "Certificate is not valid (self signed certificate)")

      elem(:public_key.pkix_path_validation(ca_root_cert, [certificate], []), 1) == :error ->
        {:error, {:bad_cert, reason }} = :public_key.pkix_path_validation(ca_root_cert, [certificate], [])
        IO.puts("    ! Verifying the certificate chain raised an error (#{reason})")
        reject_connection(conn, 495, "Certificate validation raised an error (#{reason})")

      true ->
        IO.puts("    @ certificate not self-signed")
        IO.puts("    @ trust chain validates")
        IO.puts("    + certificate constraints met")
        sig_64 = cert_signature |> Base.encode64()
        IO.puts("  + authenticated CN(#{tls_common_name}) with certificate(#{sig_64})")
        conn
    end

  end

  def call(conn, _opts) do
    reject_connection(conn, 496, "Client certificate required")
  end


  # Simple function to turn an ASN.1 UTC time into an Elixir DateTime
  defp cert_time_to_date_time({:utcTime, t}) do
    # https://hexdocs.pm/timex/Timex.Format.DateTime.Formatters.Strftime.html#content
    #           Timex.Parse.DateTime.Parser.parse(t, "%y%m%0d%H%M%SZ", :strftime)
    {:ok, dt} = Timex.Parse.DateTime.Parser.parse(to_string(t), "%y%m%0d%H%M%SZ", :strftime)
    dt
  end

  # Simple function to turn an IETF RFC5280 GeneralizedTime into an Elixir DateTime
  defp cert_time_to_date_time({:generalizedTime, t}) do
    # TODO: We need to add a method of parsing the generalized time format of RFC5280, which
    # is used for any certs with not_before or not_after dates beyond year 2049
    :not_implemented
  end

  defp reject_connection(conn, code, msg) do
    conn
      |> put_status(code)
      |> json(
          %{
            error: :true,
            msg: msg,
            timestamp: DateTime.to_string(DateTime.utc_now())
          }
         )
  end

end