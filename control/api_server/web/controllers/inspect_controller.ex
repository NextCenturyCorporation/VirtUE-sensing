defmodule ApiServer.InspectController do
  @moduledoc """
  Find and enumerate the set of sensors observing the Virtue, User,
  or Resource according to incoming targeting.

  @author: Patrick Dwyer (patrick.dwyer@twosixlabs.com)
  @date: 2017/10/30
  """
  use ApiServer.Web, :controller

  import UUID, only: [uuid4: 0]

  import ApiServer.ExtractionPlug, only: [extract_targeting: 2, extract_targeting_scope: 2]
  import ApiServer.TargetingUtils, only: [summarize_targeting: 1]

  plug :extract_targeting when action in [:inspect]
  plug :extract_targeting_scope when action in [:inspect]

  @doc """
  Return the set of sensors observing the resources described
  by the targeting selectors. This is a list of zero or more
  sensors.

  This is a _Plug.Conn handler/2_.

  The JSON response will looke like:

  ```json
   {
     "targeting": { ... k/v map of targeting selectors ... },
     "error": false,
     "timestamp": "YYYY-MM-DD HH:MM:SS.mmmmmmZ",
     "sensors": [
       {
         "sensor": sensor uuid,
         "virtue": virtue containing sensor/observed by sensor,
         "state": ( active || inactive ),
         "name": sensor human readable name
       }
     ]
   }
  ```

  ### Validations:

  ### Available data:

   - conn::assigns::targeting - key/value propery map of target selectors

  ### Returns:

   - HTTP 200 / JSON document describing a set of sensors
  """
  def inspect(conn, opts) do

    # Gather the targeted sensors
    ApiServer.TargetingUtils.log_targeting(conn.assigns.targeting, conn.assigns.targeting_scope)
    {:ok, sensors} = ApiServer.TargetingUtils.select_sensors_from_targeting(conn.assigns.targeting, conn.assigns.targeting_scope)

    # well, this is easy, isn't it? We only need to make sure we sanitize the sensor records before
    # releasing them.
    inspection_results(conn, sensors)
  end

  def inspect_all(conn, opts) do

    ApiServer.TargetingUtils.log_targeting()

    # gather all of the sensors
    sensors = ApiServer.Sensor |> ApiServer.Repo.all

    # stream the results
    inspection_results(conn, sensors)

  end

  defp inspection_results(conn, sensors) do
    conn
    |> put_status(200)
    |> json(
         Map.merge(
           %{
             "error": :false,
             "timestamp": DateTime.to_string(DateTime.utc_now()),
             "sensors": Enum.map(sensors, fn (s) -> ApiServer.Sensor.clean_json(s) end)
           },
           summarize_targeting(conn.assigns)
         )
       )
  end


end