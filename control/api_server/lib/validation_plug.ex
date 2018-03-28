defmodule ApiServer.ValidationPlug do
  @moduledoc """
  Routing action validation routines. These are executed inline to routing
  before the final handlers are called as Plug methods. Used inline with
  a Controller with:

  import ApiServer.ValidationPlug, only: [valid_observe_level: 2]
  :plug :valid_observe_level when action in [:observe]

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """

  import Phoenix.Controller
  import Plug.Conn


  @doc"""
  Validate that the OBSERVE level passed into an API endpoint is an acceptable value.

  ### Requires URI or QUERY parameters:

    - :level

  ### assigns

    conn::assigns::observation_level

  ### Default

    None

  ### Returns:

    - Continue if valid
    - HTTP 400 / HALT / JSON with error if invalid
  """
  def valid_observe_level(%Plug.Conn{params: %{"level" => level}} = conn, _) do

    # simple check, halt and catch fire
    case level do
      level when level in ["off", "default", "low", "high", "adversarial"] ->
        conn
          |> assign(:observation_level, level)

      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid observation level [#{level}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Error Case
  #
  #   missing log :level
  #
  # Response:
  #
  #   - HTTP 400 / Halt / JSON with error
  def valid_observe_level(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           msg: "Missing observation level",
           timestamp: DateTime.to_string(DateTime.utc_now())
         }
       )
    |> Plug.Conn.halt()
  end


  @doc """
  Make sure the action of a TRUST command is valid

  ### Requires URI or QUERY parameters:

    - :action

  ### assigns

    conn::assigns::trust_action

  ### Default

    None

  ### Response:

    - Continue if valid
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_trust_action(%Plug.Conn{params: %{"action" => action}} = conn, _) do

    # simple check, as usual
    case action do
      action when action in ["validate", "invalidate"] ->
        conn
          |> assign(:trust_action, action)
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid trust action [#{action}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Error Case
  #
  #   missing log :action
  #
  # Response:
  #
  #   - HTTP 400 / Halt / JSON
  def valid_trust_action(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           msg: "Missing trust action",
           timestamp: DateTime.to_string(DateTime.utc_now())
         }
       )
    |> Plug.Conn.halt()
  end


  @doc """
  Make sure the action of a validate command is valid

  ### Requires URI or QUERY parameters:

    - :action

  ### assigns

    conn::assigns::validate_action

  ### default

    None

  ### Response:

    - Continue if valid, putting action in conn::assigns:::validate_action
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_validate_action(%Plug.Conn{params: %{"action" => action}} = conn, _) do
    case action do
      action when action in ["canary", "cross-validation"] ->
        case action do
          "canary" ->
            conn
              |> assign(:validate_action, "canary-validate")
          "cross-validation" ->
            conn
              |> assign(:validate_action, "cross-validate")
        end

      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid validate action [#{action}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Error Case
  #
  #   missing log :action
  #
  # Response:
  #
  #   - HTTP 400 / Halt / JSON
  def valid_validate_action(conn, _) do
    conn
    |> put_status(400)
    |> json(
         %{
           error: :true,
           msg: "Missing validate action",
           timestamp: DateTime.to_string(DateTime.utc_now())
         }
       )
    |> Plug.Conn.halt()
  end


  @doc """
  Make sure the incoming log level parameter is valid

  ### Requires URI or QUERY parameters:

    - :filter_level

  ### assigns

    conn::assigns::filter_level

  ### default

    "everything"

  ### Response:

    - Continue if valid, putting log level in conn:assigns::filter_level
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_log_level(%Plug.Conn{params: %{"filter_level" => filter_level}} = conn, _) do
    case filter_level do
      filter_level when filter_level in ["everything", "debug", "info", "warning", "error", "event"] ->
        conn
          |> assign(:filter_level, String.to_atom(filter_level))
      filter_level when filter_level == nil ->
        IO.puts("reverting filter_level to [everything]")
        conn
          |> assign(:filter_level, :everything)
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid log filter level [#{filter_level}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
          |> Plug.Conn.halt()
    end
  end

  # Missing filter_level case
  #
  #   missing log :filter_level
  #
  # Response:
  #
  #   - HTTP 200 / Halt / JSON - implied everything
  def valid_log_level(conn, _) do
    IO.puts("no filter_level specified, defaulting to [everything]")
    conn
      |> assign(:filter_level, :everything)
  end

  @doc """
  Make sure the incoming **follow** parameter is valid.

  ### Requires URI or QUEYR parameters

    - :follow

  ### assigns

    conn::assgins::follow_log

  ### default

    :false

  ### Response

    - Continue if valid
    - Continue with default **false** if missing
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_follow(%Plug.Conn{params: %{"follow" => follow}} = conn, _) do
    case String.to_atom(String.downcase(follow)) do
      :true ->
        conn
          |> assign(:follow_log, true)
      :false ->
        conn
          |> assign(:follow_log, false)
      _ ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid value for follow [#{follow}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
    end
  end

  def valid_follow(conn, _) do
    conn
      |> assign(:follow_log, false)
  end

  @doc """
  Make sure the incoming **since** parameter is valid.

  ### Requires **since** URI or Query parameters:

    - since

  ### assigns

    conn::assigns::since_datetime

  ### default

    UTC NOW

  ### Response

    - Continue if value
    - Continue with default UTC NOW if missing
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_since(%Plug.Conn{params: %{"since" => since}} = conn, _) do

    case DateTime.from_iso8601(since) do
      {:ok, dt, tz_offset} ->
        conn
          |> assign(:since_datetime, dt)
      {:error, :invalid_format} ->
        conn
          |> put_status(400)
          |> json(
              %{
                error: :true,
                msg: "Invalid format of since parameter [#{since}]",
                timestamp: DateTime.to_string(DateTime.utc_now())
              }
             )
           |> Plug.Conn.halt()
        {:error, :missing_offset} ->
          conn
            |> put_status(400)
            |> json(
                 %{
                   error: :true,
                   msg: "Missing TZ offset in since parameter [#{since}]",
                   timestamp: DateTime.to_string(DateTime.utc_now())
                 }
               )
            |> Plug.Conn.halt()
          {:error, :invalid_time} ->
            conn
            |> put_status(400)
            |> json(
                 %{
                   error: :true,
                   msg: "Invalid time specified in since parameter [#{since}]",
                   timestamp: DateTime.to_string(DateTime.utc_now())
                 }
               )
            |> Plug.Conn.halt()
          {:error, :invalid_date} ->
            conn
            |> put_status(400)
            |> json(
                 %{
                   error: :true,
                   msg: "Invalid date specified in since parameter [#{since}]",
                   timestamp: DateTime.to_string(DateTime.utc_now())
                 }
               )
            |> Plug.Conn.halt()
    end
  end

  # Set the default *since_datetime* to UTC->NOW if not specified in query
  def valid_since(conn, _) do
    conn
      |> assign(:since_datetime, DateTime.utc_now())
  end

  @doc """
  Make sure the incoming `os` parameter is valid

  ### Checks

    - os

  ### assigns

    - component_os

  ### default

    - linux

  ### Response

    - Continue if value
    - Continue with default 'linux' if missing
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_component_os(%Plug.Conn{params: %{"os" => raw_os}} = conn, _) do
    os = String.downcase(raw_os)
    valid_os = os in ApiServer.Constants.sensor_os()
    case os do
      os when valid_os ->
        conn
        |> assign(:component_os, os)
      os when os == nil ->
        conn
        |> assign(:component_os, "linux")
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid component os [#{os}]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end
  end

  def valid_component_os(conn, _) do
    conn
    |> assign(:component_os, "linux")
  end

  @doc """
  Make sure the incoming context parameter is valid

  ### Checks

    - context

  ### Assigns

    - component_context

  # default

    - virtue

  # Response

    - Continue if value
    - Continue with default `virtue` if missing
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_component_context(%Plug.Conn{params: %{"context": raw_context}} = conn, _) do
    context = String.downcase(raw_context)
    case context do
      context when context in ["virtue", "unikernel", "hypervisor"] ->
        conn
        |> assign(:component_context, context)
      context when context == nil ->
        conn
        |> assign(:component_context, "virtue")
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid component context [#{context}]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end
  end

  def valid_component_context(conn, _) do
    conn
    |> assign(:component_context, "virtue")
  end

  @doc """
  Make sure the incoming configuration level is valid

  ### Checks

    - level

  ### Assigns

    - configuration_level

  ### Default

    - default

  ### Response

    - Continue if valid
    - Continue with default 'default' if missing
    - HTTP 400 / Halt / JSON if invalid
  """
  def valid_configuration_level(%Plug.Conn{params: %{"level": raw_level}} = conn, _) do
    level = String.downcase(raw_level)

    case level do
      level when level in ["off", "default", "low", "high", "adversarial"] ->
        conn
        |> assign(:configuration_level, level)
      level when level == nil ->
        conn
        |> assign(:configuration_level, "default")
      _ ->
        conn
        |> put_status(400)
        |> json(
             %{
               error: :true,
               msg: "Invalid configuration level [#{level}]",
               timestamp: DateTime.to_string(DateTime.utc_now())
             }
           )
        |> Plug.Conn.halt()
    end
  end

  def valid_configuration_level(conn, _) do
    conn
    |> assign(:configuration_level, "default")
  end
end