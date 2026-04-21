# opentelemetry-collector v0.68 Release Notes

Source: [v0.68.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.68.0)

### Collector images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.68.0

### Collector builder binaries here: https://github.com/open-telemetry/opentelemetry-collector/releases/tag/cmd%2Fbuilder%2Fv0.68.0

## v1.0.0-RC2/v0.68.0

### 🛑 Breaking changes 🛑

- `componenttest`: Move NopFactories to otelcoltest (#6792)
- `config/confighttp`: Change confighttp.HTTPClientSettings.Headers type to map[string]configopaque.String (#5653)
- `config`: Remove deprecated `component.Config.[ID|SetIDName]`. (#4714)
- `configauth`: Remove deprecated funcs/types from `configauth` (#6719)
- `component`: Remove deprecated funcs/types from component package (#6769)
  - `component.[Exporter|Processor|Receiver|Extension]Config`
  - `component.Unmarshal[Exporter|Processor|Receiver|Extension]Config`
  - `component.[Exporter|Processor|Receiver|Extension]CreateDefaultConfigFunc`
  - `component.[Exporter|Receiver|Extension]FactoryOption`
  - `component.New[Exporter|Receiver|Extension]Factory`
  - `component.With[Traces|Metrics|Logs][Exporter|Receiver]`
  - `component.Create[Traces|Metrics|Logs][Exporter|Receiver]Func`
  - `component.CreateExtensionFunc`
- `componenttest`: Remove deprecated componenttest.NewNop*CreateSettings (#6761)
- `service`: Remove deprecated `service.[Collector|New|CollectorSettings|ConfigProvider]` (#5564)
- `service`: Remove deprecated funcs `service.NewCommand` and `service.NewSvcHandler`. (#5564)
- `obsreporttest`: Remove deprecate obsreporttest.Check* (#6720)
- `service`: Remove deprecated `service.Config`. (#6774)
- `servicetest`: Remove deprecated `servicetest` package. (#5564)

### 🚩 Deprecations 🚩

- `service`: Deprecate `service.ConfigService` in favor of `service.Config` and `service.ConfigServicePipeline` in favor of `service.PipelineConfig`. (#6787)
- `pdata`: Deprecate `pcommon.Value.Equal` method (#6811)
- `component`: Deprecate `Processor` related structs and functions in favor of `processor` package (#6709)
- `component`: Deprecate component.Factories in favor of otelcol.Factories (#6723)
- `config`: Deprecate `config.[Extension|Exporter|Connector|Receiver|Processor]Settings`. (#6718)
- `batchprocessor`: Deprecate metric views funcs, for OC and Otel. (#6730)
- `obsreporttest`: Deprecate obsreporttest.SetupTelemetryWithID in favor of obsreporttest.SetupTelemetry (#6720)

### 🚀 New components 🚀

- `forwardconnector`: Add forward connector (#6763)

### 💡 Enhancements 💡

- `components`: Add [receiver|processor|exporter|extension].Builder to help with creating components form a set of configs and factories (#6803)
- `configunmarshaler`: Consolidate package into generic implementation (#6801)
- `service`: Shutdown internal telemetry with the Service (every time config changes). (#5564)

### 🧰 Bug fixes 🧰

- `configgrpc`: Fix todo to add MeterProvider to grpc instrumentation (#4030)
- `otlpreceiver`: Fix otlpreceiver transport metrics attribute (#6695)
