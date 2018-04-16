defmodule ApiServer.Constants do
  @moduledoc """
  Useful constants that shouldn't be litered around the system.
  """

  def sensor_os() do
    ["linux", "windows", "nt", "rump"]
  end

end