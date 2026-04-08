# opentelemetry-collector v0.53 Release Notes

Source: [v0.53.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.53.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.53.0

### 🛑 Breaking changes 🛑

- Remove deprecated `componenterror` package. (#5420)
- Remove deprecated `config.MapConverterFunc`. (#5419)
- Remove `AddCollectorVersionTag`, enabled for long time already. (#5471)

### 🚩 Deprecations 🚩

- Move `config.Map` to its own package `confmap` which does not depend on any component concept (#5237)
  - `config.Map` -> `confmap.ConfMap`
  - `config.MapProvider` -> `confmap.Provider`
  - `config.Received` -> `confmap.Received`
  - `config.NewReceivedFromMap` -> `confmap.NewReceived`
  - `config.CloseFunc` -> `confmap.CloseFunc`
  - `config.ChangeEvent` -> `confmap.ChangeEvent`
  - `config.MapConverter` -> `confmap.Converter`
  - Package `envmapprovider` -> `envprovider`
  - Package `filemapprovider` -> `fileprovider`
  - Package `yamlmapprovider` -> `yamlprovider`
  - Package `expandmapconverter` -> `expandconverter`
  - Package `filemapprovider` -> `fileprovider`
  - Package `overwritepropertiesmapconverter` -> `overwritepropertiesconverter`
- Deprecate `component.ExtensionDefaultConfigFunc` in favor of `component.ExtensionCreateDefaultConfigFunc` (#5451)
- Deprecate `confmap.Received.AsMap` in favor of `confmap.Received.AsConf` (#5465)
- Deprecate `confmap.Conf.Set`, not used anywhere for the moment (#5485)

### 💡 Enhancements 💡

- Move `service.mapResolver` to `confmap.Resolver` (#5444)
- Add `linux-arm` architecture to cross build tests in CI (#5472)

### 🧰 Bug fixes 🧰

- Fixes the "service.version" label value for internal metrics, always was "latest" in core/contrib distros. (#5449).
- Send correct batch stats when SendBatchMaxSize is set (#5385)
