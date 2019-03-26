#
# Plugs for extracting parameters from incoming request paths
# for insertion into the active connection.
#
# Called as:
#
#   import ApiServer.ParameterPlug, only: [extract_sensor_id: 2]
#   :plug :extract_sensor_id when action in [:observe]
#
# @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
# @date: 2017/10/30
defmodule ApiServer.ExtractionPlug do
  import Phoenix.Controller
  import Plug.Conn
#  import CIDR

  @doc """
  Determine the outer scope of a targeting request.

  For the current v1 API, this is the virst path element
  after the `/api/v1` portion of the URI path.

  ### Side effects

    conn.assigns.targeting_scope = (:virtue | :application | ...)

  ### Returns

    Plug.Conn

  """
  def extract_targeting_scope(%Plug.Conn{path_info: path_info} = conn, _) do

    conn
      |> assign(:targeting_scope, hd(tl(tl(path_info))))

  end

  # Extract a Sensor ID labeled `:sensor` from the incoming
  # request path.
  #
  # Requires URI or QUERY parameters:
  #   - :sensor
  #
  # Response:
  #   - Continue if valid, assign Sensor ID to conn::assigns::sensor_id
  #   - Halt/HTTP 400 if invalid
  def extract_sensor_id(%Plug.Conn{params: %{"sensor" => sensor}} = conn, _) do

    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      String.match?(sensor, ~r/[a-zA-Z0-9]{8}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{12}/) ->
        assign(conn, :sensor_id, sensor)
      true ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid or missing sensor ID",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Extract targeting used in Validation check URIs. This can be
  # one of many different values:
  #
  #     - CIDR - :cidr
  #     - VIRTUE ID - :virtue
  #     - Username - :user
  #     - Address - :address
  #     - Application - :application
  #     - Resource - :resource
  #     - sensor - :sensor
  #
  # It is possible for this to be a nested target in some URIs, with
  # the following possibilities:
  #
  #     - :virtue && :user
  #     - :virtue && :application
  #     - :user && :application
  #     - :virtue && :user && :application
  #
  # The targetting data will be assigned to conn::assigns::targeting
  #
  # Response:
  #   - continue if valid, targeting data assigned to conn::assigns::targeting
  #   - Halt/HTTP 400 if invalid

  # --- virtue && user && application
  def extract_targeting(%Plug.Conn{params: %{"virtue" => virtue, "user" => username, "application" => application}} = conn, _) do
    # we're going to construct a with statement containing our three targeting
    # values, which will drop through to a default failure case.
    with :true <- is_virtue_id(virtue),
         :true <- is_username(username),
         :true <- is_application_id(application) do
      assign(conn, :targeting, %{
        :virtue => virtue,
        :username => username,
        :application => application
      })
    else
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "One or more invalid targeting selectors in [ virtue = #{virtue}, username = #{username}, application = #{application} ]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # --- virtue && user
  def extract_targeting(%Plug.Conn{params: %{"virtue" => virtue, "user" => username}} = conn, _) do
    # we're going to construct a with statement containing our three targeting
    # values, which will drop through to a default failure case.
    with :true <- is_virtue_id(virtue),
         :true <- is_username(username) do
      assign(conn, :targeting, %{
        :virtue => virtue,
        :username => username
      })
    else
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "One or more invalid targeting selectors in [ virtue = #{virtue}, username = #{username}]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end

  end

  # --- virtue && application
  def extract_targeting(%Plug.Conn{params: %{"virtue" => virtue, "application" => application}} = conn, _) do
    # we're going to construct a with statement containing our three targeting
    # values, which will drop through to a default failure case.
    with :true <- is_virtue_id(virtue),
         :true <- is_application_id(application) do
      assign(conn, :targeting, %{
        :virtue => virtue,
        :application => application
      })
    else
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "One or more invalid targeting selectors in [ virtue = #{virtue}, application = #{application} ]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end
  end

  # --- user && application
  def extract_targeting(%Plug.Conn{params: %{"user" => username, "application" => application}} = conn, _) do
    # we're going to construct a with statement containing our three targeting
    # values, which will drop through to a default failure case.
    with :true <- is_username(username),
         :true <- is_application_id(application) do
      assign(conn, :targeting, %{
        :username => username,
        :application => application
      })
    else
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "One or more invalid targeting selectors in [ username = #{username}, application = #{application} ]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end
  end

  # --- CIDR
  def extract_targeting(%Plug.Conn{params: %{"cidr" => cidr}} = conn, _) do
    case CIDR.parse(cidr) do
      {:error, _} ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid CIDR address specified",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt
        _ ->
          assign(conn, :targeting, %{:cidr => cidr})
    end
  end

  # --- Virtue ID (uuid)
  def extract_targeting(%Plug.Conn{params: %{"virtue" => virtue}} = conn, _) do
    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      is_virtue_id(virtue) ->
        assign(conn, :targeting, %{:virtue => virtue})
      true ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid Virute ID (UUID) used for targeting",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt
    end
  end

  # --- Username (unbound character set - needs definition)
  def extract_targeting(%Plug.Conn{params: %{"user" => username}} = conn, _) do
    assign(conn, :targeting, %{:username => username})
  end

  # --- Address ( IP or Hostname)
  #
  # We're using the RFC 1123 regular expression for validating the hostname,
  # and a simple regex for matching IPv4 addresses, which falls into
  # using the Erlang INET library for actual IP validation
  def extract_targeting(%Plug.Conn{params: %{"address" => address}} = conn, _) do

    cond do
      # really loose IPv4 match
      String.match?(address, ~r/^(([0-9]{1,3}\.){3}([0-9]{1,3}))$/) ->
        case :inet.parse_address(to_charlist(address)) do
          {:error, _} ->
            conn
              |> put_status(400)
              |> json(
                  %{
                    error: :true,
                    msg: "Invalid IP address supplied for targeting",
                    timestamp: DateTime.to_string(DateTime.utc_now())
                  }
                 )
              |> Plug.Conn.halt()
            _ -> assign(conn, :targeting, %{:address => address})
        end
      # RFC 1123 hostname match
      is_hostname(address) ->
        assign(conn, :targeting, %{:address => address})
      true ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid IP address or Hostname specified for address based targeting",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt
    end
  end

  # --- Application (uuid)
  def extract_targeting(%Plug.Conn{params: %{"application" => application}} = conn, _) do
    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      is_application_id(application) ->
        assign(conn, :targeting, %{:application => application})
      true ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid Application ID (UUID) used for targeting",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt
    end
  end

  # --- Resource (uuid)
  def extract_targeting(%Plug.Conn{params: %{"resource" => resource}} = conn, _) do
    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      is_resource_id(resource) ->
        assign(conn, :targeting, %{:resource=> resource})
      true ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid Resource ID (UUID) used for targeting",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt
    end
  end

  # --- Sensor (uuid)
  def extract_targeting(%Plug.Conn{params: %{"sensor" => sensor}} = conn, _) do
    # we'll pattern match on a UUID for now, which will generally look
    # like:
    #   a8bfe405-5e0d-4acb-a3a5-c47e6fecd608
    cond do
      is_sensor_id(sensor) ->
        assign(conn, :targeting, %{:sensor => sensor})
      true ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid sensor ID (UUID) used for targeting",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt
    end
  end

  # Simple wrapper method for adding an HTTP return code to a connection, and
  # returning the connection chain.
  def conn_invalid_parameter(conn) do
    conn
    |> put_status(400)
  end

  # Is the given string a valid sensor ID?
  def is_sensor_id(st) do
    String.match?(st, ~r/^(([a-zA-Z0-9]|[a-zA-Z0-9][a-zA-Z0-9\-]*[a-zA-Z0-9])\.)*([A-Za-z0-9]|[A-Za-z0-9][A-Za-z0-9\-]*[A-Za-z0-9])$/)
  end

  # Is the given string a valid username in VirtUE?
  def is_username(st) do
    String.match?(st, ~r/^.+$/)
  end

  # Is the given string a valid Hostname according to RFC 1123 specifications?
  def is_hostname(st) do
    String.match?(st, ~r/^(([a-zA-Z0-9_]|[a-zA-Z0-9_][a-zA-Z0-9\-_]*[a-zA-Z0-9_])\.)*([A-Za-z0-9_]|[A-Za-z0-9_][A-Za-z0-9\-_]*[A-Za-z0-9_])$/)
  end

  # Is the given string a valid Virtue ID?
  # Per NC, a Virtue ID could be a FQDN, so this check is quite permissive
  def is_virtue_id(st) do
    String.match?(st, ~r/^[a-zA-Z0-9\.\-_]+$/)
  end

  # Is the given string a valid Application ID?
  def is_application_id(st) do
    is_uuid?(st)
  end

  def is_resource_id(st) do
    is_uuid?(st)
  end

  def is_public_key(st) do

    case :public_key.pem_decode(st) do
      [_] ->
        :true
      [] ->
        :false
    end
  end

  def is_sensor_port(p) when is_integer(p) do
    :true
  end

  def is_sensor_port(p) do

    parsable = Integer.parse(p)

    case p do
      p when parsable != :error -> true
      _ -> false
    end
  end

  defp is_uuid?(st) do
    String.match?(st, ~r/^[a-zA-Z0-9]{8}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{4}\-[a-zA-Z0-9]{12}$/)
  end


end