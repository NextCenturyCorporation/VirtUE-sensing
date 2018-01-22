defmodule ApiServer.ConfigurationsController do
  @moduledoc """
  Manage the available configurations for controllers. This is a set of administrative actions
  to read, update, and discover available configurations for all of the different sensor types.
  """

  use ApiServer.Web, :controller

  import ApiServer.ValidationPlug, only: [valid_component_os: 2, valid_component_context: 2, valid_configuration_level: 2]

  plug :valid_component_os when action in [:list_components, :get_component, :list_configurations, :get_configuration]
  plug :valid_component_context when action in [:list_components, :get_component, :list_configurations, :get_configuration]
  plug :valid_configuration_level when action in [:get_configuration]

  @doc """
  List all of the components/sensors matching the selectors.

  ### Validations:

    - valid_component_os
    - valid_component_context

  ### Available

    - conn::assigns::component_os
    - conn::assigns::component_context

  ### Return

    - HTTP 200 / JSON: Set of matching components
    - HTTP 400 / JSON: Error message if component selector error

  """
  def list_components(%Plug.Conn{method: "GET"} = conn, _) do

    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately, as we're OK with missing values
    filter_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context"])

    # turn the keyword list back into a map with atom keys
    filter = Enum.into(filter_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs
    full_components = ApiServer.Component.get_components(filter)

    # trimmed down and nice maps
    components = Enum.map(full_components, &clean_component/1)

    conn
    |> put_status(200)
    |> json(
        %{
          error: false,
          components: components,
          timestamp: DateTime.to_string(DateTime.utc_now())
        }
       )
  end

  @doc """

  Retrieve a component and its default configuration data.

  ### Validations:

    - valid_component_os
    - valid_component_context

  ### Available

    - conn::assigns::component_selectors - key/value map of component selectors

  ### Return

    - HTTP 200 / JSON: Matching component
    - HTTP 400 / JSON: Error message if component selector error
    - HTTP 400 / JSON: More than one matching component
    - HTTP 400 / JSON: No matching components

  """
  def get_component(%Plug.Conn{method: "GET"} = conn, _) do

    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately, as we're OK with missing values
    filter_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])

    # turn the keyword list back into a map with atom keys
    filter = Enum.into(filter_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs
    case ApiServer.Component.get_component(filter) do
      :multiple_matches ->
        conn |> error_response(400, "More than one matching component")
      :no_matches ->
        conn |> error_response(400, "No matching components")
      {:ok, component} ->
        conn
        |> put_status(200)
        |> json(
            %{
              error: false,
              timestamp: DateTime.to_string(DateTime.utc_now()),
              component: clean_component(component)
            }
           )
    end

  end

  @doc """
  Delete an existing component.

  All of our values come from the body params, and should map to a safe subset of possible
  keys and values in a Component struct.

  ### Validations:

  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: OK - component deleted
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: Invalid component configuration

  """
  def delete_component(%Plug.Conn{method: "DELETE"} = conn, _) do
    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately, as we're OK with missing values
    filter_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])

    # turn the keyword list back into a map with atom keys
    filter = Enum.into(filter_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs
    case ApiServer.Component.get_component(filter) do
      :multiple_matches ->
        conn |> error_response(400, "More than one matching component")
      :no_matches ->
        conn |> error_response(400, "No matching components")
      {:ok, component} ->

        # so we got a component, now delete it
        case ApiServer.Repo.delete(component) do
          {:ok, d_component} ->
            conn
            |> put_status(200)
            |> json(
                %{
                  error: false,
                  timestamp: DateTime.to_string(DateTime.utc_now()),
                  msg: "Component and configurations deleted"
                }
               )

          {:error, changeset} ->
            conn |> error_response_changeset(400, changeset)
        end
    end
  end



  @doc """
  Update an existing component.

  All of our values come from the body params, and should map to a safe subset of possible
  keys and values in a Component struct.

  ### Validations:

  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: OK - updated component content
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: Invalid component configuration

  """
  def update_component(%Plug.Conn{method: "PUT"} = conn, _) do

    # Extract the os, context, name, and description
    component_keywords = Keyword.take(Map.to_list(conn.body_params), ["os", "context", "name", "description"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # get our existing component
    case ApiServer.Component.get_component(component_map) do

      :no_matches ->
        conn |> error_response(400, "No matching component")
      :multiple_matches ->
        conn |> error_response(400, "More than one matching component")
      {:ok, component} ->

        # ok - we've got a component we can update. Let's do this
        case ApiServer.Component.update_component(component, component_map, save: true) do

          # Great - we matched and updated a component
          {:ok, u_component} ->
            conn
            |> put_status(200)
            |> json(
                %{
                  error: false,
                  timestamp: DateTime.to_string(DateTime.utc_now()),
                  component: clean_component(u_component)
                }
               )

          # there were issues with our update, let's feed them back
          {:error, changeset} ->
            conn |> error_response_changeset(400, changeset)
        end
    end
  end

  @doc """
  Create a new component.

  ### Validations:

  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: OK - created component content
    - HTTP 400 / JSON: Component already exists
    - HTTP 400 / JSON: Invalid component configuration

  """
  def create_component(conn, _) do
    # Extract the os, context, name, and description
    component_keywords = Keyword.take(Map.to_list(conn.body_params), ["os", "context", "name", "description"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # get our existing component
    case ApiServer.Component.get_component(component_map) do

      {:ok, component}->
        conn |> error_response(400, "Component already exists")
      :multiple_matches ->
        conn |> error_response(400, "More than one matching component")
      :no_matches ->

        # ok - we've got a new component we can create. Let's do this
        case ApiServer.Component.create(component_map, save: true) do

          # Great - we matched and updated a component
          {:ok, u_component} ->
            conn
            |> put_status(200)
            |> json(
                 %{
                   error: false,
                   timestamp: DateTime.to_string(DateTime.utc_now()),
                   component: clean_component(u_component)
                 }
               )

          # there were issues with our update, let's feed them back
          {:error, changeset} ->
            conn |> error_response_changeset(400, changeset)
        end
    end
  end

  @doc """
  List all of the configurations for a targeted component.

  ### Validations:

    - valid_component_os
    - valid_component_context

  ### Available

    - conn::assigns::component_selectors - key/value map of component selectors, component_name

  ### Return

    - HTTP 200 / JSON: configuration list
    - HTTP 400 / JSON: More than one component matching
    - HTTP 400 / JSON: No component matching

  """
  def list_configurations(conn, _) do

    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately, as we're OK with missing values
    filter_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])

    # turn the keyword list back into a map with atom keys
    filter = Enum.into(filter_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs
    case ApiServer.Component.get_component(filter) do
      :multiple_matches ->
        conn |> error_response(400, "More than one matching component")
      :no_matches ->
        conn |> error_response(400, "No matching components")
      {:ok, component} ->

        # great - let's get the list of configurations
        conn
        |> put_status(200)
        |> json(
             %{
               error: false,
               timestamp: DateTime.to_string(DateTime.utc_now()),
               component: clean_component(component),
               configurations: Enum.map(ApiServer.Component.get_configurations(component), &clean_configuration/1)
             }
           )
    end
  end

  @doc """
  Get a single configuration for a component and observation level.

  ### Validations:

    - valid_component_os
    - valid_component_context
    - valid_configuration_level

  ### Available

    - conn::assigns::component_selectors - key/value map of component selectors, component name
    - conn::assigns::configuration_selectors - configuration level

  ### Return

    - HTTP 200 / JSON: Configuration matching the component and observation level
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: No configuration available

  """
  def get_configuration(conn, _) do

    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately with NAME, as we're OK with missing values. We'll separately
    # build out the LEVEL and VERSION for querying into the configuration
    component_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])
    config_keywords = Keyword.take(Map.to_list(conn.params), ["level", "version"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)
    config_map = Enum.into(config_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs await us
    case ApiServer.Component.get_configuration(component_map, config_map) do
      :no_component_matches ->
        conn |> error_response(400, "No matching component")
      :multiple_component_matches ->
        conn |> error_response(400, "Multiple matching components")
      :no_such_configuration->
        conn |> error_response(400, "No matching configuration")
      {:ok, config} ->
        conn
        |> put_status(200)
        |> json(
             %{
               error: false,
               timestamp: DateTime.to_string(DateTime.utc_now()),
               component: clean_component(config.component),
               configuration: clean_configuration(config)
             }
           )
    end

  end

  @doc """

  ### Validations:

  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: Ok - updated configuration
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: More than one or no matching configuration
  """
  def delete_configuration(%Plug.Conn{method: "DELETE"} = conn, _) do

    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately with NAME, as we're OK with missing values. We'll separately
    # build out the LEVEL and VERSION for querying into the configuration
    component_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])
    config_keywords = Keyword.take(Map.to_list(conn.params), ["level", "version"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)
    config_map = Enum.into(config_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs await us
    case ApiServer.Component.get_configuration(component_map, config_map) do
      :no_component_matches ->
        conn |> error_response(400, "No matching component")
      :multiple_component_matches ->
        conn |> error_response(400, "Multiple matching components")
      :no_such_configuration->
        conn |> error_response(400, "No matching configuration")
      {:ok, config} ->

        # we've got the configuration, let's delete it
        case ApiServer.Repo.delete(config) do
          {:ok, d_config} ->
            conn
            |> put_status(200)
            |> json(
                %{
                  error: false,
                  timestamp: DateTime.to_string(DateTime.utc_now()),
                  msg: "Configuration deleted"
                }
               )
          {:error, changeset} ->
            conn |> error_response_changeset(400, changeset)
        end
    end
  end

  @doc """

  ### Validations:

  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: Ok - updated configuration
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: More than one or no matching configuration

  """
  def update_configuration(conn, _) do
    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately with NAME, as we're OK with missing values. We'll separately
    # build out the LEVEL and VERSION for querying into the configuration
    component_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])
    config_keywords = Keyword.take(Map.to_list(conn.params), ["level", "version", "configuration", "format", "description"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)
    config_map = Enum.into(config_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    # full ecto structs await us
    case ApiServer.Component.get_configuration(component_map, config_map) do
      :no_component_matches ->
        conn |> error_response(400, "No matching component")
      :multiple_component_matches ->
        conn |> error_response(400, "Multiple matching components")
      :no_such_configuration->
        conn |> error_response(400, "No matching configuration")
      {:ok, config} ->

        # great - we've got a configuration, let's try and update it
        case ApiServer.Configuration.update_configuration(config, config_map, save: true) do
          {:ok, u_config} ->
            conn
            |> put_status(200)
            |> json(
                %{
                  error: false,
                  timestamp: DateTime.to_string(DateTime.utc_now()),
                  component: clean_component(config.component),
                  configuration: clean_configuration(u_config)
                }
               )
          {:error, changeset} ->
            conn |> error_response_changeset(400, changeset)
        end
    end
  end

  @doc """

  ### Validations:


  ### Available

    - body params

  ### Return

    - HTTP 200 / JSON: Ok - created configuration
    - HTTP 400 / JSON: More than one or no matching component
    - HTTP 400 / JSON: Configuration already exists

  """
  def create_configuration(conn, _) do
    # we know our OS and CONTEXT have passed validation, but we need to build them out
    # as query parameters separately with NAME, as we're OK with missing values. We'll separately
    # build out the LEVEL and VERSION for querying into the configuration
    component_keywords = Keyword.take(Map.to_list(conn.params), ["os", "context", "name"])
    config_keywords = Keyword.take(Map.to_list(conn.params), ["level", "version", "configuration", "format", "description"])

    # turn the keyword list back into a map with atom keys
    component_map = Enum.into(component_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)
    config_map = Enum.into(config_keywords, %{}, fn {k, v} -> {String.to_atom(k), v} end)

    case ApiServer.Component.get_component(component_map) do
      :no_matches ->
        conn |> error_response(400, "No matching components")
      :multiple_matches ->
        conn |> error_response(400, "Multiple matching components")
      {:ok, component} ->

        # great, we have a component, let's look up this configuration
        case ApiServer.Component.get_configuration(component, config_map) do
          {:ok, config} ->
            conn |> error_response(400, "Configuration already exists")
          :no_such_configuration ->

            # woot - let's create it
            case ApiServer.Component.add_configuration(component, config_map, save: true) do
              {:ok, config} ->
                conn
                |> put_status(200)
                |> json(
                    %{
                      error: false,
                      timestamp: DateTime.to_string(DateTime.utc_now()),
                      component: clean_component(component),
                      configuration: clean_configuration(config)
                    }
                   )
                {:error, changeset} ->
                  conn |> error_response_changeset(400, changeset)
            end
        end
    end
  end

  defp clean_component(component) do
    Map.drop(Map.from_struct(component), [:"__meta__", :configurations, :id])
  end

  defp clean_configuration(config) do
    Map.drop(Map.from_struct(config), [:"__meta__", :component, :id, :component_id])
  end

  defp error_response(conn, code, msg) do
    conn
    |> put_status(code)
    |> json(
        %{
          error: true,
          msg: msg,
          timestamp: DateTime.to_string(DateTime.utc_now())
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