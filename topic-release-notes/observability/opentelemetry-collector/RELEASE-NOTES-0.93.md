# opentelemetry-collector v0.93 Release Notes

Source: [v0.93.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.93.0)

## End user Changelog

### 🛑 Breaking changes 🛑

- `exporterhelper`: remove deprecated exporterhelper.RetrySettings and exporterhelper.NewDefaultRetrySettings (#9256)
- `configopaque`: configopaque.String implements `fmt.Stringer` and `fmt.GoStringer`, outputting [REDACTED] when formatted with the %s, %q or %#v verbs` (#9213)
  This may break applications that rely on the previous behavior of opaque strings with `fmt.Sprintf` to e.g. build URLs or headers.
  Explicitly cast the opaque string to a string before using it in `fmt.Sprintf` to restore the previous behavior.
  

### 🚀 New components 🚀

- `extension/memory_limiter`: Introduce a `memory_limiter` extension which receivers can use to reject incoming requests when collector doesn't have enough memory (#8632)
  The extension has the same configuration interface and behavior as the existing `memory_limiter` processor, which potentially can be deprecated and removed in the future

### 💡 Enhancements 💡

- `configtls`: add `cipher_suites` to configtls. (#8105)
  Users can specify a list of cipher suites to pick from. If left blank, a safe default list is used.
  
- `service`: mark `telemetry.useOtelForInternalMetrics` as stable (#816)
- `exporters`: Cleanup log messages for export failures (#9219)
  1. Ensure an error message is logged every time and only once when data is dropped/rejected due to export failure.
  2. Update the wording. Specifically, don't use "dropped" term when an error is reported back to the pipeline.
     Keep the "dropped" wording for failures happened after the enabled queue.
  3. Properly report any error reported by a queue. For example, a persistent storage error must be reported as a storage error, not as "queue overflow".
  

### 🧰 Bug fixes 🧰

- `configgrpc`: Update dependency to address a potential crash in the grpc instrumentation (#9296)
- `otlpreceiver`: Ensure OTLP receiver handles consume errors correctly (#4335)
  Make sure OTLP receiver returns correct status code and follows the receiver contract (gRPC)
- `zpagesextension`: Remove mention of rpcz page from zpages extension (#9328)

## Go API Changelog

### 🛑 Breaking changes 🛑

- `bug_fix`: Implement `encoding.BinaryMarshaler` interface to prevent `configopaque` -> `[]byte` -> `string` conversions from leaking the value (#9279)
- `configopaque`: configopaque.String implements `fmt.Stringer` and `fmt.GoStringer`, outputting [REDACTED] when formatted with the %s, %q or %#v verbs` (#9213)
  This may break applications that rely on the previous behavior of opaque strings with `fmt.Sprintf` to e.g. build URLs or headers.
  Explicitly cast the opaque string to a string before using it in `fmt.Sprintf` to restore the previous behavior.
  
- `all`: Remove obsolete "// +build" directives (#9304)
- `connectortest`: Remove deprecated connectortest router helpers. (#9278)

### 🚩 Deprecations 🚩

- `obsreporttest`: deprecate test funcs/structs (#8492)
  The following methods/structs have been moved from obsreporttest to componenttest:
  - obsreporttest.TestTelemetry -> componenttest.TestTelemetry
  - obsreporttest.SetupTelemetry -> componenttest.SetupTelemetry
  - obsreporttest.CheckScraperMetrics -> TestTelemetry.CheckScraperMetrics
  - obserporttest.TestTelemetry.TelemetrySettings -> componenttest.TestTelemetry.TelemetrySettings()
  
- `confignet`: Deprecates `DialContext` and `ListenContext` functions. Use `Dial` and `Listen` instead. (#9258)
  Unlike the previous `Dial` and `Listen` functions, the new `Dial` and `Listen` functions take a `context.Context` like `DialContext` and `ListenContext`.