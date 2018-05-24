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
end