defmodule ApiServer.ErrorView do
  use ApiServer.Web, :view

  defimpl Plug.Exception, for: FunctionClauseError do
    def status(_), do: IO.puts("OH CRAP")
  end

  def render("404.html", _assigns) do
    %{
      error: :true,
      msg: "Not found",
      timestamp: DateTime.to_string(DateTime.utc_now())
    }
  end

  def render("400.html", _assigns) do
    %{
      error: :true,
      msg: "Bad request",
      timestamp: DateTime.to_string(DateTime.utc_now())
    }
  end

  def render("500.html", _assigns) do
    %{
      error: :true,
      msg: "No such route",
      timestamp: DateTime.to_string(DateTime.utc_now())
    }
  end

  # In case no render clause matches or no
  # template is found, let's render it as 500
  def template_not_found(_template, assigns) do
    render "500.html", assigns
  end
end
