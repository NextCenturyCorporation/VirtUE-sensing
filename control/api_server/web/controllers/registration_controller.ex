
defmodule ApiServer.RegistrationController do
  @moduledoc """

  The RegistrationController provides methods for registering sensors
  with the API.

  Called as:

     VERB "/path/to/:action", ApiServer.RegisterController, :register, name: "route-name"

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/11/07
  """

  use ApiServer.Web, :controller
  import ApiServer.ExtractionPlug, only: [extract_sensor_id: 2, is_virtue_id: 1, is_sensor_id: 1, is_hostname: 1, is_username: 1, is_public_key: 1, is_sensor_port: 1]

  # get our Sensor ID into conn::sensor_id
  plug :extract_sensor_id when action in [:review, :configure]

  @doc """
  Deregister a sensor with the sensing API.

  This is a _Plug.Conn handler/2_.

  The sensor **must** be identified by sensor_id and public_key to
  successfully deregister.

  ### Validations:

  ### Available:

    - conn::assigns::sensor_id

  ### Return:

    - HTTP 200 / Sensor removed from tracking
    - HTTP 400 / Invalid or missing deregistration parameters in payload
    - HTTP 500 / Deregistration error
  """
  def deregister(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "sensor" => sensor_id,
            "public_key" => public_key_b64
          }
        } = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)


    # de-registration consists of a few steps:
    #
    # 1. sensor_id validation
    # 2. public_key validation
    # 3. record deregistration
    # 4. c2 announcement

    IO.puts("Deregistering sensor(id=#{sensor_id})")

    with true <- is_sensor_id(sensor_id),
      true <- is_public_key(public_key),
      true <- ApiServer.AuthenticationUtils.matches_client_certificate(conn, public_key),
         {:ok, sensor} <- ApiServer.Sensor.get(%{public_key: public_key}),
         {:ok, sensor_d} <- ApiServer.Sensor.delete(sensor)
      do

        IO.puts("Deregistration successful sensor_id(#{sensor_id})")
        # let's announce the deletion on c2
        ApiServer.ControlUtils.announce_deregistered_sensor(sensor)

        # ok - send our response, we're good
        conn
        |> put_status(200)
        |> json(
          %{
            error: false,
            timestamp: DateTime.to_string(DateTime.utc_now()),
            sensor: sensor_id,
            deregistered: true
          }
           )

      else

        false ->
          IO.puts("Sensor validation error in deregistration sensor_id(#{sensor_id})")
          conn |> invalid_deregistration(sensor_id, "Sensor validation error")

        :no_matches ->
          IO.puts("Couldn't find sensor_id(#{sensor_id}) by public key lookup")
          conn |> invalid_deregistration(sensor_id, "Couldn't find sensor_id(#{sensor_id}) by public key lookup")

        :multiple_matches ->
          IO.puts("Found multiple sensors matching public key in deregistration (this shouldn't happen)")
          conn |> invalid_deregistration(sensor_id, "Found multiple sensors matching public key in deregistration (this shouldn't happen)")
    end

  end

  @doc """
  Sync a sensor and it's registration record, updating the registration time stamp.

  Sensors that fall too far out of sync will be automatically deregistered.

  This is a _Plug.Conn handler/2_.

  ### Validations

  ### Available

    - conn::assigns::sensor_id

  ### Return

    - HTTP 200 / Sensor synced
    - HTTP 400 / Invalid or missing sync parameters in payload
    - HTTP 410 / Sensor not currently registered or already deregistered
  """
  def sync(%Plug.Conn{method: "PUT", body_params: %{"sensor" => sensor_id, "public_key" => public_key_b64}} = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)

    # basic logging
    IO.puts("Syncing sensor(id=#{sensor_id})")

    # we'll walk through a few validations, culminating in the sync action, from which
    # we an choose which response to generate. We identify our sensor record based on
    # it's public key.
    with true <- is_sensor_id(sensor_id),
      true <- is_public_key(public_key),
      true <- ApiServer.AuthenticationUtils.matches_client_certificate(conn, public_key),
         {:ok, sensor} <- ApiServer.Sensor.get(%{public_key: public_key}),
         {:ok, sensor_synced} <- ApiServer.Sensor.sync(sensor, save: true)
      do
        # we're good
        conn
        |> put_status(200)
        |> json(
            %{
              error: false,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              sensor: sensor_id,
              synchronized: true
            }
           )
      else
        # ruhroh
        false ->
          # something didn't validate in our is_* clauses
          IO.puts("Sensor validation error sensor_id(#{sensor_id})")
          conn |> sync_error(410, "Sensor validation error sensor_id(#{sensor_id})")

        :no_matches ->
          IO.puts("No sensor matching public key for sensor_id(#{sensor_id})")
          conn |> sync_error(410, "No sensor matching public key for sensor_id(#{sensor_id})")

        :multiple_matches ->
          IO.puts("Multiple matching sensors for public key (this shouldn't be possible)")
          conn |> sync_error(410, "Multiple matching sensors for public key (this shouldn't be possible)")

        {:error, changeset} ->
          IO.puts("Error updating sync in database for sensor_id(#{sensor_id})")
          IO.puts(Poison.encode!(changeset_errors_json(changeset)))
          conn |> sync_error(410, "Error updating sync in database for sensor_id(#{sensor_id})")

    end

  end

  defp sync_error(conn, status, reason) do
    conn
    |> put_status(status)
    |> json(
        %{
          error: true,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          synchronized: false,
          msg: reason
        }
       )
  end

  @doc """
  Register a sensor with the Sensing API

  This is a _Plug.Conn handler/2_.

  ### Validations:

  ### Available:

    - conn::assigns::sensor_id

  ### Return:

    - HTTP 200 / Sensor registered
    - HTTP 400 / Invalid or missing registration parameters in payload
    - HTTP 500 / Registration error
  """
  def register(
        %Plug.Conn{
          method: "PUT",
          body_params: %{
            "sensor" => sensor,
            "virtue" => virtue,
            "user" => username,
            "public_key" => public_key_b64,
            "hostname" => hostname,
            "port" => port,
            "name" => sensor_name_version
          } = body_params
        } = conn, _) do

    # let's decode the public key from urlsafe base64
    {:ok, public_key} = Base.url_decode64(public_key_b64)

    # our default OS for registration is linux
    sensor_os = Map.get(body_params, "os", "linux")

    # basic logging
    IO.puts("Registering sensor(id=#{sensor})")

    # strip down our sensor name into a useful string, without the trailing version portion
    sensor_name = sensor_name_version
      |> String.split("-")
      |> List.delete_at(-1)
      |> Enum.join("-")

    cond do

      # we have a bunch of failure cases up front
      ! is_sensor_id(sensor) ->
        invalid_registration(conn, "-", "sensor", sensor)
      ! is_virtue_id(virtue) ->
        invalid_registration(conn, sensor, "virtue", virtue)
      ! is_hostname(hostname) ->
        invalid_registration(conn, sensor, "hostname", hostname)
      ! is_username(username) ->
        invalid_registration(conn, sensor, "username", username)
      ! is_public_key(public_key) ->
        invalid_registration(conn, sensor, "public_key", public_key)
      ! ApiServer.AuthenticationUtils.matches_client_certificate(conn, public_key) ->
        invalid_registration(conn, sensor, "public_key", "Client certificate and provided public key don't match")
      ! is_sensor_port(port) ->
        IO.puts IEx.Info.info(port)
        invalid_registration(conn, sensor, "port", port)
      ! ApiServer.Component.has_default_configuration?(%{name: sensor_name, context: "virtue", os: sensor_os}) ->
        invalid_registration(conn, sensor, "default configuration", "Cannot locate default configuration for #{sensor_name}")

      # now we have a valid registration
      :true ->

        # can we verify that this remote sensor is actually listening for actuation?
        case verify_remote_sensor(hostname, port, sensor, public_key) do

          # remote is listening, let's register it
          :ok ->

            # our registration process is multi-step, and we'll catch the error conditions at the
            # end of the with/1 block. For registration we need to:
            #
            #   1. Get our sensor component
            #   2. Create our sensor object
            #   3. Assign the component to the sensor
            #   4. Assign a default configuration
            #   5. Retrieve the configuration
            #   6. Mark the sensor as registered
            #   ?. Verify public key against the PKIKey record in mnesia
            with {:ok, component} <- ApiServer.Component.get_component(%{name: sensor_name, context: "virtue", os: sensor_os}),
              {:ok, sensor_create} <- ApiServer.Sensor.create(
                %{
                  sensor_id: sensor,
                  virtue_id: virtue,
                  username: username,
                  address: hostname,
                  port: port,
                  public_key: public_key
                }, save: true),
              {:ok, sensor_comp} <- ApiServer.Sensor.assign_component(sensor_create, component, save: true),
              {:ok, sensor_conf} <- ApiServer.Sensor.assign_configuration(sensor_comp, %{}, save: true),
              {:ok, configuration} <- ApiServer.Sensor.get_configuration_content(sensor_conf),
              {:ok, sensor_reg} <- ApiServer.Sensor.mark_as_registered(sensor_conf, save: true),
              {:ok, sensor_cert} <- ApiServer.Sensor.mark_has_certificates(sensor_reg, save: true),
              {:ok, authchallenge} <- ApiServer.AuthChallenge.get(%{public_key: public_key}),
              {:ok, linked_authchallenge} <- ApiServer.Sensor.add(sensor_cert, authchallenge, save: true)
              do
                # in theory everything worked, so we can build out our response, but we should still preload
                # our configuration data
                sensor_record = ApiServer.Repo.preload(sensor_cert, [:configuration, :component, :authchallenges])

                # send out an alert on the C2 channel about a new sensor
                ApiServer.ControlUtils.announce_new_sensor(sensor_record)

                # record some registration metadata to the log
                IO.puts("  @ sensor(id=#{sensor}, name=#{sensor_name}) registered at #{DateTime.to_string(DateTime.utc_now())}")
                IO.puts("  <> sensor(id=#{sensor}) assigned kafka topic(id=#{sensor_record.kafka_topic})")

                # send out the registration response
                conn
                |> put_status(200)
                |> json(
                     %{
                       error: :false,
                       timestamp: DateTime.to_string(DateTime.utc_now()),
                       sensor: sensor,
                       registered: :true,
                       kafka_bootstrap_hosts: Application.get_env(:api_server, :sensor_kafka_bootstrap),
                       sensor_topic: sensor_record.kafka_topic,
                       default_configuration: sensor_record.configuration.configuration
                     }
                   )

            else

              # quite a few of our error conditions involve changeset data
              {:error, changeset} ->
                IO.puts("  error registering sensor")
                IO.puts(changeset_errors_json(changeset))
                conn |> error_response_changeset(400, changeset_errors_json(changeset))

            end

          # remote isn't responding, this is a problem
          :error ->

            conn
              |> put_status(417) # timeout: expectation failed
              |> json(
                   %{
                    error: :true,
                    timestamp: DateTime.to_string(DateTime.utc_now()),
                    msg: "Could not contact the sensor with a verification ping, registration incomplete"
                   }
                 )

        end

    end
  end

  def register(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "One or more missing fields in the registration payload"
         }
       )
  end

  @doc """
  Send a verification ping to a remote sensor.

  During the regstration process, sensors are verified via side channel round trip HTTP
  calls. If the sensor returns an HTTP/200, we can complete the registration. The sensor
  ID is sent as part of the HTTP request, which the sensor should verify as being it's
  correct sensor ID.

  ### Params:

    - **hostname**: bare hostname of sensor, without any leading scheme
    - **port**: integer port monitored by sensor
    - **sensor**: ID of the sensor being verified

  ### Returns:

    - :ok - sensor verified
    - :error - verification failed
  """
  def verify_remote_sensor(hostname, port, sensor, pinned_key) do

    # root CA to the HTTPoison hackney instance, with something like:
    #
    #   HTTPoison.get("https://example.com/", [], ssl: [cacertfile: "/app/certs/ca.pem"])
    #
    # see: https://github.com/edgurgel/httpoison/issues/294

    # let's send out our verification ping
    verification_url = "https://#{hostname}:#{port}/sensor/#{sensor}/registered"
    IO.puts("  = remote verifcation uri(#{verification_url})")
    case HTTPoison.get(verification_url, [], [ssl: [{:cacertfile, Application.get_env(:api_server, :ca_cert_file)}, {:verify_fun, {&ApiServer.AuthenticationUtils.pin_verify/3, {:pin, pinned_key}}}, {:verify, :verify_peer}], timeout: 5000, recv_timeout: 5000, connect_timeout: 5000]) do

      {:ok, %HTTPoison.Response{status_code: 200, body: _}} ->
        IO.puts("  + sensor(id=#{sensor}) verified with direct ping")
        :ok
      {:ok, %HTTPoison.Response{status_code: 401}} ->
        IO.puts("  - sensor(id=#{sensor}) verification mismatch from direct ping")
        :error
      {:ok, %HTTPoison.Response{status_code: 500}} ->
        IO.puts("  - internal server error from sensor(id=#{sensor}) during verification ping")
        :error
      {:ok, %HTTPoison.Response{status_code: status_code}} ->
        IO.puts("  - status_code == #{status_code} from sensor(id=#{sensor}) during verification ping")
        :error
      {:error, %HTTPoison.Error{reason: reason}} ->
        IO.puts("  ! HTTPoison error during sensor(id=#{sensor}) verification ping: #{reason}")
        :error
    end
  end

  defp invalid_sync(conn, sensor_id, key, value) do
    IO.puts("! sensor(id=#{sensor_id}) failed synchronization with (#{key} = #{value})")
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "sensor(id=#{sensor_id}) failed synchronization with (#{key} = #{value})"
         }
       )
    |> Plug.Conn.halt()
  end

  # Build the HTTP/400 JSON response to an invalid registration attempt. This call
  # will halt the connection.
  defp invalid_registration(conn, sensor_id, fieldname, value) do
    IO.puts("! sensor(id=#{sensor_id}) failed registration with invalid field (#{fieldname} = #{value})")
    conn
    |> put_status(400)
    |> json(
         %{
          error: :true,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          msg: "sensor(id=#{sensor_id}) failed registration with invalid field (#{fieldname} = #{value})"
         }
       )
    |> Plug.Conn.halt()
  end

  # Build the HTTP/400 JSON response to an invalid deregistration request. This call will
  # halt the connection.
  defp invalid_deregistration(conn, sensor_id, message) do
    IO.puts("! sensor(id=#{sensor_id}) failed deregistration: #{message}")
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           msg: "sensor(id=#{sensor_id}) failed deregistration: #{message}"
         }
       )
    |> Plug.Conn.halt()
  end

  # Generate an error message in JSON from an Ecto changeset featuring error data,
  # using Ecto.Changeset.traverse_errors
  defp error_response_changeset(conn, code, changeset) do
    conn
    |> put_status(code)
    |> json(
         %{
           error: true,
           timestamp: DateTime.to_string(DateTime.utc_now()),
           errors: changeset_errors_json(changeset)
         }
       )
    |> Plug.Conn.halt()
  end

  defp changeset_errors_json(c) do
    Ecto.Changeset.traverse_errors(c, fn {msg, opts} ->
      Enum.reduce(opts, msg, fn {key, value}, acc ->
        String.replace(acc, "%{#{key}}", to_string(value))
      end)
    end)
  end

end