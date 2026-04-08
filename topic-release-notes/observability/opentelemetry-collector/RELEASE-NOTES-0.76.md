# opentelemetry-collector v0.76 Release Notes

Source: [v0.76.1](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.76.1)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.76.1

### 🛑 Breaking changes 🛑

- `confmap`: Using an Invalid Scheme in a URI will throw an error. (#7504)

### 🚩 Deprecations 🚩

- `featuregate`: Deprecate Gate.RemovalVersion and WithRegisterRemovalVersion in favor of ToVersion. (#7043)

### 💡 Enhancements 💡

- `batchprocessor`: Support zero timeout. (#7508)
  This allows the batchprocessor to limit request sizes without introducing delay in a pipeline, to act only as a splitter.
- `service`: use the otel opencensus bridge when telemetry.useOtelForInternalMetrics is enabled (#7483)
- `connector`: Mark 'service.connectors' featuregate as stable (#2336)
- `featuregate`: Add a new Deprecated stage for feature gates, when features are abandoned. (#7043)
- `loggingexporter`: Show more counters in not detailed verbosity (#7461)
  The logging exporter now shows more counters when the verbosity is not detailed. The following numbers are added:
  - Number of resource logs
  - Number of resource spans
  - Number of resource metrics
  - Number of data points
  
- `configtls`: Reload mTLS ClientCA certificates on file change (#6524)
- `confmap`: Add support for nested URIs. (#7117)
- `featuregate`: Add concept of gate lifetime, [fromVersion, toVersion]. (#7043)

### 🧰 Bug fixes 🧰

- `obsreport`: fix issue where send_failed_requests counter was reporting an incorrect value. (#7456)
