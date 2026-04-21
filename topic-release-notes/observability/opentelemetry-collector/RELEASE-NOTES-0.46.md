# opentelemetry-collector v0.46 Release Notes

Source: [v0.46.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.46.0)


## v0.46.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.46.0

### 🛑 Breaking changes 🛑

- Deprecated funcs `config.DefaultConfig`, `confighttp.DefaultHTTPSettings`, `exporterhelper.DefaultTimeoutSettings`, 
  `exporthelper.DefaultQueueSettings`, `exporterhelper.DefaultRetrySettings`, `testcomponents.DefaultFactories`, and
  `scraperhelper.DefaultScraperControllerSettings` in favour for their `NewDefault` method to adhere to contribution guidelines (#4865)
- Deprecated funcs `componenthelper.StartFunc`, `componenthelper.ShutdownFunc` in favour of `component.StartFunc` and `component.ShutdownFunc` (#4803)
- Move helpers from extensionhelper to component (#4805)
  - Deprecated `extensionhelper.CreateDefaultConfig` in favour of `component.ExtensionDefaultConfigFunc`
  - Deprecated `extensionhelper.CreateServiceExtension` in favour of `component.CreateExtensionFunc`
  - Deprecated `extensionhelper.NewFactory` in favour of `component.NewExtensionFactory`
- Move helpers from processorhelper to component (#4889)
  - Deprecated `processorhelper.CreateDefaultConfig` in favour of `component.ProcessorDefaultConfigFunc`
  - Deprecated `processorhelper.WithTraces` in favour of `component.WithTracesProcessor`
  - Deprecated `processorhelper.WithMetrics` in favour of `component.WithMetricsProcessor`
  - Deprecated `processorhelper.WithLogs` in favour of `component.WithLogsProcessor`
  - Deprecated `processorhelper.NewFactory` in favour of `component.NewProcessorFactory`
- Move helpers from exporterhelper to component (#4899)
  - Deprecated `exporterhelper.CreateDefaultConfig` in favour of `component.ExporterDefaultConfigFunc`
  - Deprecated `exporterhelper.WithTraces` in favour of `component.WithTracesExporter`
  - Deprecated `exporterhelper.WithMetrics` in favour of `component.WithMetricsExporter`
  - Deprecated `exporterhelper.WithLogs` in favour of `component.WithLogsExporter`
  - Deprecated `exporterhelper.NewFactory` in favour of `component.NewExporterFactory`
- Move helpers from receiverhelper to component (#4891)
  - Deprecated `receiverhelper.CreateDefaultConfig` in favour of `component.ReceiverDefaultConfigFunc`
  - Deprecated `receiverhelper.WithTraces` in favour of `component.WithTracesReceiver`
  - Deprecated `receiverhelper.WithMetrics` in favour of `component.WithMetricsReceiver`
  - Deprecated `receiverhelper.WithLogs` in favour of `component.WithLogsReceiver`
  - Deprecated `receiverhelper.NewFactory` in favour of `component.NewReceiverFactory`
- Change otel collector to enable open telemetry metrics through feature gate instead of a constant
- Remove support for legacy otlp/http port. (#4916)
- Remove deprecated funcs in pdata (#4809)
- Remove deprecated Retrieve funcs/calls (#4922)
- Remove deprecated NewConfigProvider funcs (#4937)

### 💡 Enhancements 💡

- Add validation to check at least one endpoint is specified in otlphttpexporter's configuration (#4860)

## 🧰 Bug fixes 🧰

- Initialized logger with collector to avoid potential race condition panic on `Shutdown` (#4827)
- In addition to traces, now logs and metrics processors will start the memory limiter.
  Added thread-safe logic so only the first processor can launch the `checkMemLimits` go-routine and the last processor
  that calls shutdown to terminate it; this is done per memory limiter instance.
  Added memory limiter factory to cache initiated object and be reused by similar config. This guarantees a single
  running `checkMemLimits` per config (#4886)
- Resolved race condition in collector when calling `Shutdown` (#4878)
