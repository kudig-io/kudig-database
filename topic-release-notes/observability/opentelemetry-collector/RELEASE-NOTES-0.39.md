# opentelemetry-collector v0.39 Release Notes

Source: [v0.39.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.39.0)

## 🛑 Breaking changes 🛑

- Remove deprecated config (already no-op) `ballast_size_mib` in memorylimiterprocessor (#4365)
- Remove `config.Receivers`, `config.Exporters`, `config.Processors`, and `config.Extensions`. Use map directly (#4344)
- Remove `component.BaseProcessorFactory`, use `processorhelper.NewFactory` instead (#4175)
- Force usage of `exporterhelper.NewFactory` to implement `component.ExporterFactory` (#4338)
- Force usage of `receiverhelper.NewFactory` to implement `component.ReceiverFactory` (#4338)
- Force usage of `extensionhelper.NewFactory` to implement `component.ExtensionFactory` (#4338)
- Move `service/parserprovider` package to `config/configmapprovider` (#4206)
   - Rename `MapProvider` interface to `Provider`
   - Remove `MapProvider` from helper names
- Renamed slice-valued `pdata` types and functions for consistency. (#4325)
  - Rename `pdata.AnyValueArray` to `pdata.AttributeValueSlice`
  - Rename `ArrayVal()` to `SliceVal()`
  - Rename `SetArrayVal()` to `SetSliceVal()`
- Remove `config.Pipeline.Name` (#4326)
- Rename `config.Mapprovider` as `configmapprovider.Provider` (#4337)
- Move `config.WatchableRetrieved` and `config.Retrieved` interfaces to `config/configmapprovider` package (#4337)
- Remove `config.Pipeline.InputDataType` (#4343)
- otlpexporter: Do not retry on PermissionDenied and Unauthenticated (#4349)
- Remove deprecated funcs `consumererror.As[Traces|Metrics|Logs]` (#4364)
- Remove support to expand env variables in default configs (#4366)

## 💡 Enhancements 💡
- Supports more compression methods(`snappy` and `zstd`) for configgrpc, in addition to current `gzip` (#4088)
- Moved the OpenTelemetry Collector Builder to core (#4307)

## 🧰 Bug fixes 🧰

- Fix AggregationTemporality and IsMonotonic when metric descriptors are split in the batch processor (#4389)
