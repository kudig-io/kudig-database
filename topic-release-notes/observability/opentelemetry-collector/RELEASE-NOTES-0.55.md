# opentelemetry-collector v0.55 Release Notes

Source: [v0.55.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.55.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.55.0

### 🛑 Breaking changes 🛑

- Remove deprecated `config.ServiceTelemetry` (#5565)
- Remove deprecated `config.ServiceTelemetryLogs` (#5565)
- Remove deprecated `config.ServiceTelemetryMetrics` (#5565)

### 🚩 Deprecations 🚩

- Deprecate `service.ConfigServiceTelemetry`, `service.ConfigServiceTelemetryLogs`, and `service.ConfigServiceTelemetryMetrics` (#5565)
- Deprecate the following component functions to ensure a stability level is set (#5580):
  - `component.WithTracesExporter` -> `component.WithTracesExporterAndStabilityLevel`
  - `component.WithMetricsExporter` -> `component.WithMetricsExporterAndStabilityLevel`
  - `component.WithLogsExporter` -> `component.WithLogsExporterAndStabilityLevel`
  - `component.WithTracesReceiver` -> `component.WithTracesReceiverAndStabilityLevel`
  - `component.WithMetricsReceiver` -> `component.WithMetricsReceiverAndStabilityLevel`
  - `component.WithLogsReceiver` -> `component.WithLogsReceiverAndStabilityLevel`
  - `component.WithTracesProcessor` -> `component.WithTracesProcessorAndStabilityLevel`
  - `component.WithMetricsProcessor` -> `component.WithMetricsProcessorAndStabilityLevel`
  - `component.WithLogsProcessor` -> `component.WithLogsProcessorAndStabilityLevel`

### 💡 Enhancements 💡

- Components stability levels are now logged. By default components which haven't defined their stability levels, or which are
  unmaintained, deprecated or in development will log a message. (#5580)

### 💡 Enhancements 💡

- `exporter/logging`: Skip "bad file descriptor" sync errors (#5585)

### 🧰 Bug fixes 🧰

- Fix initialization of the OpenTelemetry MetricProvider. (#5571)
- Set log level for `undefined` stability level to debug. (#5635)
