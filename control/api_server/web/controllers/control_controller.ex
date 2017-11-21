defmodule ApiServer.ControlController do
  use ApiServer.Web, :controller

  @doc """
  Return the name of the Sensing API Command and Control topic in Kafka.

  ### Returns

    - Plug.Conn
  """
  def c2_channel(conn, _) do
    conn
      |> put_status(200)
      |> json(
          %{
            error: false,
            timestamp: DateTime.to_string(DateTime.utc_now()),
            channel: Application.get_env(:api_server, :c2_kafka_topic),
            kafka_bootstrap_hosts: Application.get_env(:api_server, :client_kafka_bootstrap),
          }
         )
  end

end