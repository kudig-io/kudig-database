# opentelemetry-collector v0.89 Release Notes

Source: [v0.89.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.89.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.89.0

## User facing changes

### 💡 Enhancements 💡

- `builder`: remove replace statement in builder template (#8763)
- `service/extensions`: Allow extensions to declare dependencies on other extensions and guarantee start/stop/notification order accordingly. (#8732)
- `exporterhelper`: Log export errors when retry is not used by the component. (#8791)
- `cmd/builder`: Add --verbose flag to log `go` subcommands output that are ran as part of a build (#8715)
- `exporterhelper`: Remove internal goroutine loop for persistent queue (#8868)
- `exporterhelper`: Simplify usage of storage client, avoid unnecessary allocations (#8830)
- `exporterhelper`: Simplify logic in boundedMemoryQueue, use channels len/cap (#8829)

### 🧰 Bug fixes 🧰

- `exporterhelper`: fix bug with queue size and capacity metrics (#8682)
- `obsreporttest`: split handler for otel vs oc test path in TestTelemetry (#8758)
- `builder`: Fix featuregate late initialization (#4967)
- `service`: Fix connector logger zap kind key (#8878)

## API changes

### 🛑 Breaking changes 🛑

- `otelcol`: CollectorSettings.Factories now expects: `func() (Factories, error)` (#8478)
- `exporter/exporterhelper`: The experimental Request API is updated. (#7874)
  - `Request` interface now includes ItemsCount() method.
  - `RequestItemsCounter` is removed.
  - The following interfaces are added:
    - Added an optional interface for handling errors that occur during request processing `RequestErrorHandler`.
    - Added a function to unmarshal bytes into a Request `RequestUnmarshaler`.
    - Added a function to marshal a Request into bytes `RequestMarshaler`
  

### 🚩 Deprecations 🚩

- `featuregate`: Deprecate `featuregate.NewFlag` in favor of `featuregate.Registry`'s `RegisterFlags` method (#8727)

### 💡 Enhancements 💡

- `featuregate`: Add validation for feature gates ID, URL and versions. (#8766)
  Feature gates IDs are now explicitly restricted to ASCII alphanumerics and dots.
  
