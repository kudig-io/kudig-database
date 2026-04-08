# opentelemetry-collector v0.58 Release Notes

Source: [v0.58.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.58.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.58.0

### 🛑 Breaking changes 🛑

- Remove the InstrumentationLibrary to Scope translation (part of transition to OTLP 0.19). (#5819)
  - This has a side effect that when sending JSON encoded telemetry using OTLP proto <= 0.15.0, telemetry will be dropped.
- Require the storage to be explicitly set for the (experimental) persistent queue (#5784)
- Remove deprecated `confighttp.HTTPClientSettings.ToClientWithHost` (#5803)
- Remove deprecated component stability helpers (#5802):
  - `component.WithTracesExporterAndStabilityLevel`
  - `component.WithMetricsExporterAndStabilityLevel`
  - `component.WithLogsExporterAndStabilityLevel`
  - `component.WithTracesReceiverAndStabilityLevel`
  - `component.WithMetricsReceiverAndStabilityLevel`
  - `component.WithLogsReceiverAndStabilityLevel`
  - `component.WithTracesProcessorAndStabilityLevel`
  - `component.WithMetricsProcessorAndStabilityLevel`
  - `component.WithLogsProcessorAndStabilityLevel`
- ABI breaking change: `featuregate.Registry.Apply` returns error now.
- Update minimum go version to 1.18 (#5795)
- Remove deprecated `Flags` API from pdata (#5814)
- Change `confmap.Provider` to return pointer to `Retrieved` (#5839)

### 🚩 Deprecations 🚩

- Deprecate duplicate settings in service.ConfigProvider, embed ResolverSettings (#5843)
- Deprecate `featuregate.Registry.MustApply` in favor of `featuregate.Registry.Apply`. (#5801)
- Deprecate the `component.Factory.StabilityLevel(config.DataType)` in favor of Stability per component (#5762):
  - `component.ExporterFactory.TracesExporterStability`
  - `component.ExporterFactory.MetricsExporterStability`
  - `component.ExporterFactory.LogsExporterStability`
  - `component.ProcessorFactory.TracesProcessorStability`
  - `component.ProcessorFactory.MetricsProcessorStability`
  - `component.ProcessorFactory.LogsProcessorStability`
  - `component.ReceiverFactory.TracesReceiverStability`
  - `component.ReceiverFactory.MetricsReceiverStability`
  - `component.ReceiverFactory.LogsReceiverStability`
- Deprecate `obsreport.ProcessorSettings.Level` and `obsreport.ExporterSettings.Level`, use MetricsLevel from CreateSettings (#5824)
- Deprecate `processorhelper.New[Traces|Metrics|Logs]Processor` in favor of `processorhelper.New[Traces|Metrics|Logs]ProcessorWithCreateSettings` (#5833)
- Deprecate MetricDataPointFlags.String(), no other pdata flags have this method (#5868)
- Deprecates `FlagsStruct` in favor of `Flags` (#5842)
  - `MetricDataPointFlagsStruct` -> `MetricDataPointFlags`
  - `NewMetricDataPointFlagsStruct` -> `NewMetricDataPointFlags`
  - `FlagsStruct` -> `Flags`
- Deprecate `exporterhelper.New[Traces|Metrics|Logs]Exporter` in favor of `exporterhelper.New[Traces|Metrics|Logs]ExporterWithContext` (#5834)

### 💡 Enhancements 💡

- Enable persistent queue in the build by default (#5828)
- Bump to opentelemetry-proto v0.19.0. (#5823)
- Expose `Scope.Attributes` in pdata (#5826)
- Remove unnecessary limitation on `pcommon.Value.Equal` that slices have only primitive values. (#5865)
- Add support to handle 404, 405 http error code as permanent errors in OTLP exporter (#5827)
- Enforce scheme name restrictions to all `confmap.Provider` implementations. (#5861)