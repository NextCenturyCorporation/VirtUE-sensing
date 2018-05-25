defmodule ApiServer.C2Channel do
  use Phoenix.Channel

  def join("c2:all", _message, socket) do
    {:ok, socket}
  end

  def join("c2:heartbeat", _message, socket) do
    {:ok, socket}
  end

  def join("c2:summary", _message, socket) do
    {:ok, socket}
  end

  def join("room:" <> _private_room_id, _params, _socket) do
    {:error, %{reason: "unauthorized"}}
  end
end