defmodule ApiServer.TargetingUtils do
  alias :mnesia, as: Mnesia

  # We need to nicely format our targeting data
  def summarize_targeting(target_key_map) do
    target_key_map
  end

  @doc """
  Create a log message about what is being targeted in a target selector
  compatible query.
  """
  def log_targeting(targeting, targeting_scope) do

    IO.puts("🎯 summary")
    IO.puts("   scope: #{targeting_scope}")
    Enum.map(targeting, fn{k, v} ->
        IO.puts("    + #{k} == #{v}")
      end
    )
  end


  def select_sensors_from_targeting(targeting, targeting_scope) do

    guard = select_guard_from_targeting(targeting, targeting_scope)

    # determine our record key
    case Mnesia.transaction(
           fn ->
             case Mnesia.select(Sensor,
                    [
                      {
                        # match
                        {
                          Sensor,
                          :"$1", :"$2", :"$3", :"$4", :"$5", :"$6", :"$7", :"$8", :"$9", :"$10"
                        },

                        guard,

                        # result (full record)
                        [
                          :"$$"
                        ]
                      }
                    ]
                  ) do

               # nuthin'
               [] ->
                 IO.puts("  * no sensors matching targeting")
                 {:ok, []}

               # we've got some old records! Let's prune 'em out!
               records ->
                 IO.puts("  * #{length(records)} sensor(s) matching targeting")
                 srecs = Enum.map(records,
                   fn (rec) ->

                     # prune out the old sensor
                     rec_t = List.to_tuple([Sensor] ++ rec)

                     # announce the pruning on the C2 channel
                     Sensor.from_mnesia_record(rec_t)

                   end
                 )
                 {:ok, srecs}
             end
           end
         ) do
      {:atomic, {:ok, sensors}} ->
        {:ok, sensors}
    end
  end


  @doc """
  Build an :mnesia.select query guard to match the targeting. See README for information
  on the relative precedence of query terms.

  ### Parameters

    - Map - results of ApiServer.ExtractionPlug.extract_targeting/2
    - Targeting scope - result of ApiServer.ExtractionPlug.extract_scope/2

  ### Returns

    List of Tuples, each Tuple defining a guard condition, which has
    the form:

      {:comparator, :"$field number", value}

  """
  def select_guard_from_targeting(%{virtue: virtue, username: username, application: application}, targeting_scope) do
    IO.puts("Targeting selection guard (virtue=#{virtue}, username=#{username}, application=#{application}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with application not yet supported")
    []
  end

  def select_guard_from_targeting(%{virtue: virtue, username: username}, targeting_scope) do
    IO.puts("Targeting selection guard (virtue=#{virtue}, username=#{username}) in scope(#{targeting_scope})")
    [
      {
        :==,
        :"$3",
        virtue
      },
      {
        :==,
        :"$4",
        username
      }
    ]
  end

  def select_guard_from_targeting(%{virtue: virtue, application: application}, targeting_scope) do
    IO.puts("Targeting selection guard (virtue=#{virtue}, application=#{application}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with application not yet supported")
    []
  end

  def select_guard_from_targeting(%{username: username, application: application}, targeting_scope) do
    IO.puts("Targeting selection guard (username=#{username}, application=#{application}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with application not yet supported")
    []
  end

  def select_guard_from_targeting(%{cidr: cidr}, targeting_scope) do
    IO.puts("Targeting selection guard (cidr=#{cidr}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with CIDR not yet supported")
    []
  end

  def select_guard_from_targeting(%{virtue: virtue}, targeting_scope) do
    IO.puts("Targeting selection guard (virtue=#{virtue}) in scope(#{targeting_scope})")
    [
      {
        :==,
        :"$3",
        virtue
      }
    ]
  end

  def select_guard_from_targeting(%{username: username}, targeting_scope) do
    IO.puts("Targeting selection guard (username=#{username}) in scope(#{targeting_scope})")
    [
      {
        :==,
        :"$4",
        username
      }
    ]
  end

  def select_guard_from_targeting(%{address: address}, targeting_scope) do
    IO.puts("Targeting selection guard (address=#{address}) in scope(#{targeting_scope})")
    [
      {
        :==,
        :"$5",
        address
      }
    ]
  end

  def select_guard_from_targeting(%{application: application}, targeting_scope) do
    IO.puts("Targeting selection guard (application=#{application}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with Application not yet supported")
    []
  end

  def select_guard_from_targeting(%{resource: resource}, targeting_scope) do
    IO.puts("Targeting selection guard (resource=#{resource}) in scope(#{targeting_scope})")
    IO.puts("ERROR: targeting with Resource not yet supported")
    []
  end

  def select_guard_from_targeting(%{sensor: sensor}, targeting_scope) do
    IO.puts("Targeting selection guard (sensor=#{sensor}) in scope(#{targeting_scope})")
    [
      {
        :==,
        :"$2",
        sensor
      }
    ]
  end
end