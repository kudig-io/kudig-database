# opentelemetry-collector v0.111 Release Notes

Source: [v0.111.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.111.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.111.0

:new: The binary release adds a new [OTLP-only distro](https://github.com/open-telemetry/opentelemetry-collector-releases/tree/main/distributions/otelcol-otlp#opentelemetry-collector-otlp-distro). Feel free to leave us feedback on this new distro on the [opentelemetry-collector-releases issue tracker](https://github.com/open-telemetry/opentelemetry-collector-releases/issues/new).

## End User Changelog

### 🐛 Known bugs 🐛

- The `ocb` binary has an identified bug caused by the fact that some of the providers have been marked stable and the default providers in the `ocb` binary still use the unstable version. In order to fix this explicitly add the default providers in your otel builder config, if not already configured:

```
providers:
  - gomod: go.opentelemetry.io/collector/confmap/provider/envprovider v1.17.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/fileprovider v1.17.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/httpprovider v1.17.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/httpsprovider v1.17.0
  - gomod: go.opentelemetry.io/collector/confmap/provider/yamlprovider v1.17.0
``` 

**This release removes the `logging` exporter. See #11337 to migrate to the `debug` exporter.**

### 🛑 Breaking changes 🛑

- `service/telemetry`: Change default metrics address to "localhost:8888" instead of ":8888" (#11251)
  This behavior can be disabled by disabling the feature gate `telemetry.UseLocalHostAsDefaultMetricsAddress`.
- `loggingexporter`: Removed the deprecated `logging` exporter.  Use the `debug` exporter instead. (#11037).
   You can read issue #11337 for migration instructions.

### 🚩 Deprecations 🚩

- `service/telemetry`: Deprecate service::telemetry::metrics::address in favor of service::telemetry::metrics::readers. (#11205)
- `processorhelper`: Deprecate BuildProcessorMetricName as it's no longer needed since introduction of mdatagen (#11302)

### 💡 Enhancements 💡

- `ocb`: create docker images for OCB, per https://github.com/open-telemetry/opentelemetry-collector-releases/pull/671 (#5712)
  Adds standard Docker images for OCB to Dockerhub and GitHub, see hub.docker.com/r/otel/opentelemetry-collector-builder
- `confighttp`: Snappy compression to lazy read for memory efficiency (#11177)
- `httpsprovider`: Mark the httpsprovider as stable. (#11191)
- `httpprovider`: Mark the httpprovider as stable. (#11191)
- `yamlprovider`: Mark the yamlprovider as stable. (#11192)
- `confmap`: Allow using any YAML structure as a string when loading configuration including time.Time formats (#10659)
  Previously, fields with time.Time formats could not be used as strings in configurations
  
### 🧰 Bug fixes 🧰

- `processorhelper`: Fix data race condition, concurrent writes to the err variable, causes UB (Undefined Behavior) (#11350)
- `cmd/builder`: re-adds function to properly set and view version number of OpenTelemetry Collector Builder (ocb) binaries (#11208)
- `pdata`: Unmarshal Span and SpanLink flags from JSON (#11267)

## API Changes


### 🛑 Breaking changes 🛑

- `service/telemetry`: Change default metrics address to "localhost:8888" instead of ":8888" (#11251)
  This behavior can be disabled by disabling the feature gate 'telemetry.UseLocalHostAsDefaultMetricsAddress'.
- `componentprofiles`: Removed deprecated `DataTypeProfiles`.  Use `SignalProfiles` instead. (#11312)
- `configgrpc`: Replace ToClientConn and ToServer with ToClientConnWithOptions and ToServerWithOptions. (#11271, #9480)
  `ClientConfig.ToClientConn` and `ServerConfig.ToServer` were deprecated in v0.110.0 in favor of
  `ClientConfig.ToClientConnWithOptions` and `ServerConfig.ToServerWithOptions` which use a more
  flexible option type. The original functions are now removed, and the new ones are renamed to the
  old names. The `WithOptions` names are kept as deprecated aliases for now.
  
- `exporterhelper`: Removed deprecated `QueueTimeout`/`TimeoutSettings` aliases in favor of `QueueConfig`/`TimeoutConfig`. (#11264, #6767)
  `NewDefaultQueueSettings` and `NewDefaultTimeoutSettings` have been similarly renamed.
- `exporterqueue`: Remove deprecated `Settings.DataType`. Use `Settings.Signal` instead. (#11305)
- `exportertest`: Remove deprecated `CheckConsumeContractParams.DataType`. Use `CheckConsumeContractParams.Signal` instead. (#11305)
- `component`: Removed deprecated `ErrDataTypeIsNotSupported`, `DataType`, `DataTypeTraces`, `DataTypeMetrics`, and `DataTypeLogs`.  Use `pipeline.ErrSignalNotSupported`, `pipeline.Signal`, `pipeline.SignalTraces`, `pipeline.SignalMetrics`, and `pipeline.SignalLogs` instead. (#11253)
- `pdata/pprofile`: Replace slices of values to slices of pointers for the `Mapping`, `Location`, `Line`, `Function`, `AttributeUnit`, `Link`, `Value`, `Sample` and `Labels` attributes. (#11339)
- `receivertest`: Remove deprecated `CheckConsumeContractParams.DataType`. Use `CheckConsumeContractParams.Signal` instead. (#11304)
- `scraperhelper`: Remove deprecated function `NewScraperWithComponentType`. (#11294)
- `processorhelper`: Remove deprecated funcs form processorhelper.ObsReport (#11289)
  The "otelcol_processor_dropped_log_records", "otelcol_processor_dropped_log_records" | and "otelcol_processor_dropped_spans" metrics are complete removed, before they were always record with 0 values.

### 🚩 Deprecations 🚩

- `componentstatus`: Deprecated `NewInstanceIDWithPipelineIDs`, `AllPipelineIDsWithPipelineIDs`, and `WithPipelineIDs`. Use `NewInstanceID`, `AllPipelineIDs`, and `WithPipelines` instead. (#11313)
- `processorhelper`: Deprecate unused and empty struct processorhelper.ObsReport (#11293)
- `processor`: Deprecate funcs that repeat "processor" in name (#11310)
  Factory.Create[Traces|Metrics|Logs|Profiles]Processor -> Factory.Create[Traces|Metrics|Logs|Profiles]
  Factory.[Traces|Metrics|Logs|Profiles]ProcessorStability -> Factory.[Traces|Metrics|Logs|Profiles]Stability
  
- `receiver`: Deprecate funcs that repeat "receiver" in name (#11287)
  Factory.Create[Traces|Metrics|Logs|Profiles]Receiver -> Factory.Create[Traces|Metrics|Logs|Profiles]
  Factory.[Traces|Metrics|Logs|Profiles]ReceiverStability -> Factory.[Traces|Metrics|Logs|Profiles]Stability
  
- `receivertest`: Deprecated `NewNopFactoryForTypeWithSignal`. Use `NewNopFactoryForType` instead. (#11304)
- `service`: Deprecates `Config.PipelinesWithPipelineID`, `pipelines.ConfigWithPipelineID` and `GetExportersWithSignal` interface implementation. Use `Config.Pipelines`, `pipelines.Config`, and `GetExporters` interface implementation instead. (#11303)

