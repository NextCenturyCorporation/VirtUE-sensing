defmodule ApiServer.BrowserConsoleController do
  use ApiServer.Web, :controller

  def index(conn, _params) do
    render conn, "index.html"
  end

  def sensors(conn, _params) do
    render conn, "sensors.html", sensors: ApiServer.Repo.all(ApiServer.Sensor) |> ApiServer.Repo.preload([:component, :configuration])
  end

  def virtues(conn, _params) do
    render conn, "virtues.html", virtues: ApiServer.Sensor.virtues()
  end

  def status(conn, _params) do
    render conn, "status.html"
  end

  def sensor(conn, %{"sensor_id" => sensor_id} = params) do

    case ApiServer.Sensor.get(%{sensor_id: sensor_id}) do
      {:ok, sensor} ->
        render conn, "sensor.html", sensor: ApiServer.Repo.preload(sensor, [:component, :configuration])
      _ ->
        redirect conn, to: "/ui"
    end

  end
end