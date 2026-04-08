# opentelemetry-collector v0.48 Release Notes

Source: [v0.48.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.48.0)

## v0.48.0 Beta

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.48.0

### 🛑 Breaking changes 🛑

- Remove deprecated `consumerhelper` package (#5028)
- Remove pdata `InternalRep` deprecated funcs (#5018)
- Remove service/defaultcomponents deprecated package (#5019)
- Remove deprecated UseOpenTelemetryForInternalMetrics (#5026)
- Change outcome of `pdata.Value.MapVal()` and `pdata.Value.SliceVal()` functions misuse. In case of
  type mismatch, they now return an invalid zero-initialized instance instead of a detached
  collection (#5034)
- OTLP JSON field changes following upgrade to OTLP v0.15.0:
  - "instrumentationLibraryLogs" is now "scopeLogs"
  - "instrumentationLibraryMetrics" is now "scopeMetrics"
  - "instrumentationLibrarySpans" is now "scopeSpans"
  - "instrumentationLibrary" is now "scope"
- AsString for pdata.Value now returns the JSON-encoded string of floats. (#4934)

### 🚩 Deprecations 🚩

- Move MapProvider to config, split providers in their own package (#5030)
- API related to `pdata.AttributeValue` is deprecated in favor of `pdata.Value` (#4978)
  - `pdata.AttributeValue` struct is deprecated in favor of `pdata.Value`
  - `pdata.AttributeValueType` type is deprecated in favor of `pdata.ValueType`
  - `pdata.AttributeValueType...` constants are deprecated in favor of `pdata.ValueType...`
  - `pdata.NewAttributeValue...` funcs are deprecated in favor of `pdata.NewValue...`
- Deprecate featureflags.FlagValue.SetSlice, unnecessary public (#5053)
- Remove "Attribute" part from common pdata collections names (#5001)
  - Deprecate `pdata.AttributeMap` struct in favor of `pdata.Map`
  - Deprecate `pdata.NewAttributeMap` func in favor of `pdata.NewMap`
  - Deprecate `pdata.NewAttributeMapFromMap` func in favor of `pdata.NewMapFromRaw`
  - Deprecate `pdata.AttributeValueSlice` struct in favor of `pdata.Slice`
  - Deprecate `pdata.NewAttributeValueSlice` func in favor of `pdata.NewSlice`
- Deprecate LogRecord.Name(), it was deprecated in the data model (#5054)
- Rename `Array` type of `pdata.Value` to `Slice` (#5066)
  - Deprecate `pdata.AttributeValueTypeArray` type in favor of `pdata.ValueTypeSlice`
  - Deprecate `pdata.NewAttributeValueArray` func in favor of `pdata.NewValueSlice`
- Deprecate global flag in `featuregates` (#5060)
- Deprecate last funcs/structs in componenthelper (#5069)
- Change structs in otlpgrpc to follow standard go encoding interfaces (#5062)
  - Deprecate UnmarshalJSON[Traces|Metrics|Logs][Reques|Response] in favor of `UnmarshalJSON`.
  - Deprecate [Traces|Metrics|Logs][Reques|Response].Marshal in favor of `MarshalProto`.
  - Deprecate UnmarshalJSON[Traces|Metrics|Logs][Reques|Response] in favor of `UnmarshalProto`.
- Deprecating following pdata methods/types following OTLP v0.15.0 upgrade (#5076):
      - InstrumentationLibrary is now InstrumentationScope
      - NewInstrumentationLibrary is now NewInstrumentationScope
      - InstrumentationLibraryLogsSlice is now ScopeLogsSlice
      - NewInstrumentationLibraryLogsSlice is now NewScopeLogsSlice
      - InstrumentationLibraryLogs is now ScopeLogs
      - NewInstrumentationLibraryLogs is now NewScopeLogs
      - InstrumentationLibraryMetricsSlice is now ScopeMetricsSlice
      - NewInstrumentationLibraryMetricsSlice is now NewScopeMetricsSlice
      - InstrumentationLibraryMetrics is now ScopeMetrics
      - NewInstrumentationLibraryMetrics is now NewScopeMetrics
      - InstrumentationLibrarySpansSlice is now ScopeSpansSlice
      - NewInstrumentationLibrarySpansSlice is now NewScopeSpansSlice
      - InstrumentationLibrarySpans is now ScopeSpans
      - NewInstrumentationLibrarySpans is now NewScopeSpans
      
### 💡 Enhancements 💡

- Add semconv definitions for v1.9.0 (#5090)
- Change outcome of `pdata.Metric.<Gauge|Sum|Histogram|ExponentialHistogram>()` functions misuse.
  In case of type mismatch, they don't panic right away but return an invalid zero-initialized
  instance for consistency with other OneOf field accessors (#5035)
- Update OTLP to v0.15.0 (#5064)
- Adding support for transition from older versions of OTLP to OTLP v0.15.0 (#5085)

### 🧰 Bug fixes 🧰

- Add missing files for semconv definitions v1.7.0 and v1.8.0 (#5091)
- The `featuregates` were not configured from the "--feature-gates" flag on windows service (#5060)
- Fix Semantic Convention Schema URL definition for 1.5.0 and 1.6.1 versions (#5103)
