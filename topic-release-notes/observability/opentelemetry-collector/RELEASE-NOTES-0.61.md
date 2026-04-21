# opentelemetry-collector v0.61 Release Notes

Source: [v0.61.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.61.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.61.0

### 🛑 Breaking changes 🛑

- Change `ptrace.Span[Link]?.TraceState` signature to match `ptrace.Span[Link]?.TraceStateStruct` (#6085)
- Delete deprecated `pmetric.NewMetricDataPointFlagsImmutable` func. (#6097)
- Delete deprecated `pmetric.*DataPoint.[Set]FlagsImmutable()` funcs. (#6097)
- Delete deprecated `config.Unmarshalable` interface. (#6084)
- Delete deprecated `p[metric|log|trace].MarshalerSizer` interfaces (#6083)
- Delete deprecated `pcommon.Map.Insert*` funcs. (#6088)
- Delete deprecated `pcommon.Map.Upsert*` funcs. (#6088)
- Delete deprecated `pcommon.Map.Update*` funcs. (#6088)
- Change `pcommon.NewValueBytes` signature to match `pcommon.NewValueBytesEmpty`. (#6088)
- Delete deprecated `pcommon.Empty[Trace|Span]ID`. (#6098)
- Delete deprecated `pcommon.[Trace|Span]ID.Bytes()`. (#6098)
- Delete deprecated `pcommon.New[Trace|Span]ID()`. (#6098)
- Delete deprecated `pcommon.Value.SetBytesVal`. (#6088)
- Delete deprecated `pmetric.Metric.SetDataType`. (#6095)
- Delete deprecated `plog.LogRecord.[Set]FlagStruct` funcs. (#6100)
- Delete deprecated `pcommon.ImmutableByteSlice` and `pcommon.NewImmutableByteSlice`. (#6107)
- Delete deprecated `pcommon.ImmutableFloat64Slice` and `pcommon.NewImmutableFloat64Slice`. (#6107)
- Delete deprecated `pcommon.ImmutableUInt64Slice` and `pcommon.NewImmutableUInt64Slice`. (#6107)
- Delete deprecated `*DataPoint.SetBucketCounts` and `*DataPoint.SetExplicitBounds`. (#6108)

### 🚩 Deprecations 🚩

- Deprecate `go.opentelemetry.io/collector/service/featuregate` in favor of `go.opentelemetry.io/collector/featuregate`. (#6094)
- Deprecate `pmetric.OptionalType`, unused enum type. (#6096)
- Deprecate `ptrace.Span[Link]?.TraceStateStruct` in favor of `ptrace.Span[Link]?.TraceState` (#6085)
- Deprecate `pcommon.NewValueBytesEmpty` in favor of `pcommon.NewValueBytes` that now has the same signature. (#6105)
- Deprecate `pmetric.MetricDataType` and related constants in favor of `pmetric.MetricType`. (#6127)
- Deprecate `pmetric.Metric.DataType()` in favor of `pmetric.Metric.Type()`. (#6127)
- Deprecate `pmetric.NumberDataPoint.[Set]?[Int|Double]Val()` in favor of `pmetric.NumberDataPoint.[Set]?[Int|Double]Value()`. (#6134)
- Deprecate `pmetric.Exemplar.[Set]?[Int|Double]Val()` in favor of `pmetric.Exemplar.[Set]?[Int|Double]Value()`. (#6134)
- Deprecate `p[metric|log|trace]otlp.[Client|Server]` in favor of `p[metric|log|trace]otlp.GRPC[Client|Server]` (#6165)
- Deprecate pdata Clone methods in favor of CopyTo for consistency with other pdata structs (#6164)
  - `pmetric.Metrics.Clone` is deprecated in favor of `pmetric.Metrics.CopyTo`
  - `ptrace.Traces.Clone` is deprecated in favor of `pmetric.Traces.CopyTo`
  - `plog.Logs.Clone` is deprecated in favor of `plogs.Logs.CopyTo`
- Rename all `pcommon.Value` getter/setter methods by removing `Val` suffix. (#6092) 
  - Old methods with `Val` suffix are deprecated.
  - `StringVal` and `SetStringVal` are deprecated in favor of `Str` and `SetStr` to avoid implementing `fmt.Stringer` interface. 
  - Therefore, `ValueTypeString` is deprecated in favour of `ValueTypeStr` for consistency.

### 💡 Enhancements 💡

- Add AppendEmpty and EnsureCapacity method to primitive pdata slices (#6060)
- Expose `AsRaw` and `FromRaw` `pcommon.Value` methods (#6090)
- Convert `ValueTypeBytes` attributes in logging exporter (#6153)
- Updated how `telemetryInitializer` is created so it's instanced per Collector instance rather than global to the process (#6138)
