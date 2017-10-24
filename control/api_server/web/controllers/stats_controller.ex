defmodule ApiServer.StatsController do
  use ApiServer.Web, :controller

  def qotds(conn, _params) do
    query = from q in ApiServer.Qotd,
              select: q.id

    stats = %{
      qotds: Repo.all(query)
    }
    json conn, stats
  end

  def qotd(conn, %{"id" => id}) do
    quote = Repo.get(ApiServer.Qotd, String.to_integer(id))
    json conn, quote
  end
end
