# opentelemetry-collector v0.77 Release Notes

Source: [v0.77.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.77.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.77.0

### 🛑 Breaking changes 🛑

- `exporterhelper`: Reduce the default queue size to 1000 from 5000 (#7359)
  Affects any exporter which enables the queue by default and doesn't set its own default size.
  For example: otlphttp.
  
- `featuregate`: Remove deprecated `RemovalVersion` and `WithRegisterRemovalVersion` functions. (#7587)

### 💡 Enhancements 💡

- `service`: Adds ResourceAttributes map to telemetry settings and thus CreateSettings. (#6599)
- `service`: Allows users to disable high cardinality OTLP attributes behind a feature flag. (#7517)
- `featuregate`: Finalize purpose of `toVersion`.  Allow stable gates to be explicitly set to true, but produce a warning log. (#7626)

### 🧰 Bug fixes 🧰

- `config/confighttp`: Ensure Auth RoundTripper follows compression/header changes (#7574)
- `otlpreceiver`: do not reject requests having 'content-type' header with optional parameters (#7452)