# Raw Commands

## create component

```elixir
component = ApiServer.Repo.insert!(ApiServer.Component.create(%{name:"kernel-ps"}))
```

## add association

```elixir
changes = component |> Ecto.build_assoc(:configurations) |> ApiServer.Configuration.changeset(%{version: "20180117"})
ApiServer.Repo.insert!(changes)
```

## Load component and configurations

```elixir
ApiServer.Repo.get(ApiServer.Component, component.id) |> ApiServer.Repo.preload(:configurations)
```

## Load component by name

```elixir
component = ApiServer.Component |> Ecto.Query.where(name: "kernel-ps") |> ApiServer.Repo.one() |> ApiServer.Repo.preload(:configurations) 
```

## Get configuration by component and version

```elixir
config = ApiServer.Configuration |> ApiServer.Configuration.for_component(component) |> Ecto.Query.where(version: "20180117") |> ApiServer.Repo.one
```

## update configuration

```elixir
ApiServer.Repo.update(ApiServer.Configuration.changeset(config, %{level: "adversarial"}))

conf = Ecto.build_assoc(component, :configurations, %{version: "20180101"})
c2 = ApiServer.Configuration.changeset(conf)
```

# Support Functions and Convenience Methods

## get a component

```elixir
component = ApiServer.Component.get_by_name("kernel-ps")
```

## Add a config version

```elixir
component |> ApiServer.Component.add_configuration(%{version: "temp"}, save: true)
```

## get a config version

```elixir
config = ApiServer.Component.get_configuration(component, "temp")
```

## update a config

```elixir
config |> ApiServer.Configuration.update_configuration(%{version: "2018-01-20T10:10:10"}, save: true)
```


## Get all configurations

```elixir
ApiServer.Configuration |> ApiServer.Repo.all
```