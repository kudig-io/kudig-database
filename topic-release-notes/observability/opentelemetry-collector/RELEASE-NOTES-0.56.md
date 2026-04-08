# opentelemetry-collector v0.56 Release Notes

Source: [v0.56.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.56.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.56.0

### 💡 Enhancements 💡

- Add `linux-ppc64le` architecture to cross build tests in CI (#5645)
- `client`: perform case insensitive lookups in case the requested metadata value isn't found (#5646)
- `loggingexporter`: Decouple `loglevel` field from level of logged messages (#5678)
- Expose `pcommon.NewSliceFromRaw` function (#5679)
- `loggingexporter`: create the exporter's logger from the service's logger (#5677)
- Add `otelcol_exporter_queue_capacity` metrics show the collector's exporter queue capacity (#5475)

### 🧰 Bug fixes 🧰

- Fix Collector panic when disabling telemetry metrics (#5642)
- Fix Collector panic when featuregate value is empty (#5663)
- Fix confighttp.compression panic due to nil request.Body. (#5628)