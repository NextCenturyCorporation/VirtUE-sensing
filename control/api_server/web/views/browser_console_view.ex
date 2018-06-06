defmodule ApiServer.BrowserConsoleView do
  use ApiServer.Web, :view

  def api_version do
    ApiServer.version()
  end

  def cert_config do
    cert_algo = Application.get_env(:api_server, :cfssl_default_algo)
    cert_size = Application.get_env(:api_server, :cfssl_default_size)
    "#{cert_algo}/#{cert_size}"
  end

  def virtue_count do
    ApiServer.Sensor.host_count()
  end

  def sensor_count do
    ApiServer.Sensor.count()
  end

  def sensor_type_count do
    ApiServer.Sensor.sensor_name_count() |> Enum.count()
  end

  def sensor_os_count do
    ApiServer.Sensor.sensor_os_count() |> Enum.count()
  end

  def elixir_info do
    [
      process_count: length(Process.list()),
      registered_count: length(Process.registered()),
      vsn: Keyword.get(Application.spec(:api_server), :vsn)
    ]
  end

end