# opentelemetry-collector v0.107 Release Notes

Source: [v0.107.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.107.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.107.0

## End User Changelog

## v1.13.0/v0.107.0

### 🛑 Breaking changes 🛑

- `service`: Remove OpenCensus bridge completely, mark feature gate as stable. (#10414)
- `confmap`: Set the `confmap.unifyEnvVarExpansion` feature gate to Stable. Expansion of `$FOO` env vars is no longer supported.  Use `${FOO}` or `${env:FOO}` instead. (#10508)
- `service`: Remove `otelcol` from Prometheus configuration. This means that any metric that isn't explicitly prefixed with `otelcol_` no longer have that prefix. (#9759)

### 💡 Enhancements 💡

- `mdatagen`: export ScopeName in internal/metadata package (#10845)
  This can be used by components that need to set their scope name manually. Will save component owners from having to store a variable, which may diverge from the scope name used by the component for emitting its own telemetry.
- `semconv`: Add v1.26.0 semantic conventions package (#10249, #10829)
- `mdatagen`: Expose a setting on tests::host to set up your own host initialization code (#10765)
  Some receivers require a host that has additional capabilities such as exposing exporters.
  For those, we can expose a setting that allows them to place a different host in the generated code.
  
- `confmap`: Allow using any YAML structure as a string when loading configuration. (#10800)
  Previous to this change, slices could not be used as strings in configuration.
  
- `ocb`: migrate build and release of ocb binaries to opentelemetry-collector-releases repository (#10710)
  ocb binaries will now be released under open-telemetry/opentelemetry-collector-releases tagged as "cmd/builder/vX.XXX.X"
- `semconv`: Add semantic conventions version v1.27.0 (#10837)
- `client`: Mark module as stable. (#10775)

### 🧰 Bug fixes 🧰

- `configtelemetry`: Add 10s read header timeout on the configtelemetry Prometheus HTTP server. (#5699)
- `service`: Allow users to disable the tracer provider via the feature gate `service.noopTracerProvider` (#10858)
  The service is returning an instance of a SDK tracer provider regardless of whether there were any processors configured causing resources to be consumed unnecessarily.
- `processorhelper`: Fix processor metrics not being reported initially with 0 values. (#10855)
- `service`: Implement the `temporality_preference` setting for internal telemetry exported via OTLP (#10745)
- `configauth`: Fix unmarshaling of authentication in HTTP servers. (#10750)
- `confmap`: If loading an invalid YAML string through a provider, use it verbatim instead of erroring out. (#10759)
  This makes the ${env:ENV} syntax closer to how ${ENV} worked before unifying syntaxes.
  
- `component`: Allow component names of up to 1024 characters in length. (#10816)
- `confmap`: Remove original string representation if invalid. (#10787)


## Go API Changelog

## v1.13.0/v0.107.0

### 🛑 Breaking changes 🛑

- `otelcol`: Delete deprecated NewCommandMustSetProvider (#10778)
- `component`: Removes the deprecated `Host.GetFactory` method. (#10771)
- `otelcoltest`: The `otelcol.LoadConfig` method no longer sets the `expandconverter`. (#10510)
- `ocb`: Collectors built with OCB will no longer include the `expandconverter` (#10510)
- `exporterhelper`: Delete deprecated `exporterhelper.ObsReport` and `exporterhelper.NewObsReport` (#10779, #10592)

### 🚩 Deprecations 🚩

- `expandconverter`: Deprecate `expandconverter`. (#10510)

### 🚀 New components 🚀

- `componentstatus`: Adds new componentstatus module that will soon replace status content in component. (#10730)
- `connector/connectorprofiles`: Allow handling profiles in connector. (#10703)
- `exporter/exporterprofiles`: Allow handling profiles in exporter. (#10702)
- `processor/processorprofiles`: Allow handling profiles in processor. (#10691)
- `receiver/receiverprofiles`: Allow handling profiles in receiver. (#10690)

### 💡 Enhancements 💡

- `confmap`: Check that providers have a correct scheme when building a confmap.Resolver. (#10786)
- `confighttp`: Add `NewDefaultCORSConfig` function to initialize the default `confighttp.CORSConfig` (#9655)
