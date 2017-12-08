
defmodule ApiServer.ConfigureController do
  @moduledoc """
  The ConfigureController provides methods for getting and setting the
  configuration of specific sensors in the SAVIOR architecture.

  Called as:

    VERB "/path/to/:action", ApiServer.Configure, :action, name: "route-name"

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """

  use ApiServer.Web, :controller
  import ApiServer.ExtractionPlug, only: [extract_sensor_id: 2]

  # get our Sensor ID into conn::sensor_id
  plug :extract_sensor_id when action in [:review, :configure]


  @doc """
  Load the CA root certificate from disk and return it to the caller.

  ### Validations:

    n/a

  ### Available:

    n/a

  ### Return:

    - HTTP 200 / CA root as JSON
    - HTTP 500 / CA root not found
  """
  def ca_root_cert(conn, _) do

    case Application.get_env(:api_server, :ca_cert_file, :does_not_exist) do

      :does_not_exist ->
        IO.puts("ConfigureController::ca_root_cert -> Cannot find configuration for locating the CA root certificate!")
        conn
          |> put_status(500)
          |> json(
              %{
                "error": true,
                "timestamp": DateTime.to_string(DateTime.utc_now()),
                "message": "Could not locate the CA root public certificate"
              }
             )
      ca_root_cert_path ->
        case File.read(ca_root_cert_path) do
          {:ok, ca_root_cert_data} ->
            conn
              |> put_status(200)
              |> json(
                  %{
                    "error": false,
                    "certificate": ca_root_cert_data,
                    "timestamp": DateTime.to_string(DateTime.utc_now())
                  }
                 )
          {:error, reason} ->
            IO.puts("ConfigureController::ca_root_cert -> error reading CA public key from #{ca_root_cert_path}: #{:file.format_error(reason)}")
            conn
              |> put_status(500)
              |> json(
                  %{
                    "error": true,
                    "timestamp": DateTime.to_string(DateTime.utc_now()),
                    "message": to_string(:file.format_error(reason))
                  }
                 )
        end
    end
  end

  @doc """
  Retrieve the settings of a specific sensor, identified by
  the sensor id :sensor in the path.

  This is a _Plug.Conn handler/2_.

  ### Validations:

  ### Available:

    - conn::assigns::sensor_id - canonical sensor ID to review

  ### Return:

    - HTTP 200 / Sensor config if valid sensor ID
    - HTTP 400 / Error message if invalid sensor ID
    - HTTP 504 / Timeout querying sensor
  """
  def review(conn, _) do
    IO.puts("ConfigureController -> Operating on sensor[#{conn.assigns.sensor_id}]")
    conn
      |> put_status(200)
      |> json(
        %{
          error: :false,
          sensor: conn.assigns.sensor_id,
          timestamp: DateTime.to_string(DateTime.utc_now()),
          configuration: %{}
        }
       )
  end


  @doc """
  Update the settings of a specific sensor, identified by
  the sensor id :sensor in the path.

  This is a _Plug.Conn handler/2_.

  The JSON request body must include:

    - configuration: URI or base64 encoded configuration data

  ### Validations

  ### Available:

    - conn::assigns::sensor_id - canonical ID of sensor to configure

  ### Return:

    - HTTP 200 / Sensor config if valid sensor ID
    - HTTP 400 / Error message if invalid sensor ID
    - HTTP 400 / Missing configuration
    - HTTP 504 / Timeout querying and updating sensor
  """
  def configure(%Plug.Conn{body_params: %{"configuration" => configuration}}=conn, _) do
    IO.puts("ConfigureController -> Operation on sensor [#{conn.assigns.sensor_id}]")

    # are we working with a URI to a config or a base64 encoding of a config
    cond do
      String.starts_with?(configuration, "http") ->
        # we've got a URI as config
        conn
         |> put_status(200)
         |> json(%{
              error: :false,
              sensor: conn.assigns.sensor_id,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              config_source: "uri",
              msg: "ok"
            })
      true ->
        # we've got base 64 as a config, which we need to try and decode to make
        # sense of
        case Base.decode64(configuration) do
          {:ok, _} ->
            conn
              |> put_status(200)
              |> json(
                  %{
                    error: :false,
                    sensor: conn.assigns.sensor_id,
                    timestamp: DateTime.to_string(DateTime.utc_now()),
                    config_source: "base64",
                    msg: "ok"
                  }
                 )
            :error ->
              conn
                |> put_status(400)
                |> json(
                    %{
                      error: :true,
                      msg: "Could not decode Base64 encoded configuration blob",
                      timestamp: DateTime.to_string(DateTime.utc_now())
                    }
                   )
        end
        conn
    end
  end

  # configuration does not exist in request body
  def configure(conn, _) do
    conn
      |> put_status(400)
      |> json(
          %{
            error: :true,
            msg: "Missing configuration payload",
            timestamp: DateTime.to_string(DateTime.utc_now())
          }
         )
  end

end