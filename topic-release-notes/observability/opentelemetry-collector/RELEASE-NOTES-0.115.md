# opentelemetry-collector v0.115 Release Notes

Source: [v0.115.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.115.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.115.0

## End User Changelog

## v1.21.0/v0.115.0

### 🛑 Breaking changes 🛑

- `otelcol`: Change all logged timestamps to ISO8601. (#10543)
  This makes log timestamps human-readable (as opposed to epoch seconds in
  scientific notation), but may break users trying to parse logged lines in the
  old format.
- `pdata/pprofile`: Upgrade pdata to opentelemetry-proto v1.4.0 (#11722)

### 🚩 Deprecations 🚩

- `scraperhelper`: Deprecate all Scraper helpers in scraperhelper (#11732)
  Deprecate ScrapeFunc, ScraperOption, WithStart, WithShutdown in favor of equivalent funcs in scraper package.

### 💡 Enhancements 💡

- `exporterqueue`: Introduce a feature gate exporter.UsePullingBasedExporterQueueBatcher to use the new pulling model in exporter queue batching. (#8122, #10368)
  If both queuing and batching is enabled for exporter, we now use a pulling model instead of a
  pushing model. num_consumer in queue configuration is now used to specify the maximum number of
  concurrent workers that are sending out the request. 
  
- `service`: label metrics as alpha to communicate their stability (#11729)
- `consumer`: Mark consumer as stable. (#9046)
- `service`: Add support for ca certificates in telemetry metrics otlp grpc exporter (#11633)
  Before this change the Certificate value in config was silently ignored.

### 🧰 Bug fixes 🧰

- `service`: ensure OTLP emitted logs respect severity (#11718)
- `featuregate`: Fix an unfriendly display message `runtime error` when featuregate is used to display command line usage. (#11651)
- `profiles`: Fix iteration over scope profiles while counting the samples. (#11688)

## API Changelog

## v1.21.0/v0.115.0

### 🛑 Breaking changes 🛑

- `extension/auth/authtest`: `authtest` is now its own module (#11465, #11705)
- `pdata/pprofile`: AttributeTable is now a slice rather than a map (#11706)
- `scraperhelper`: Remove deprecated scraperhelper funcs Scraper.ID, NewScraper, AddScraper. (#11710)
- `mdatagen`: Remove deprecated LeveledMeter from the generated code (#11696)

### 🚩 Deprecations 🚩

- `component`: Mark `TelemetrySettings.LeveledMeterProvider` as deprecated (#11697)
- `receiver/scraper`: Move receiver/scrapererror package to scraper/scrapererror and deprecate original receiver/scrapererror package. (#11003)
- `scraperhelper`: Make Scraper compatible with the new scraper.Metrics (#11682)
  Deprecate scraperhelper.Scraper in favor of scraper.Metrics