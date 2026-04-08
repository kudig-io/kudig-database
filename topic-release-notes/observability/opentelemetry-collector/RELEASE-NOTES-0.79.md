# opentelemetry-collector v0.79 Release Notes

Source: [v0.79.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.79.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.79.0

### 🚩 Deprecations 🚩

- `component`: Deprecate Host.GetExporters function (#7370)

### 💡 Enhancements 💡

- `otelcol`: Add connectors to output of the `components` command (#7809)
- `scraperhelper`: Will start calling scrapers on component start. (#7635)
  The change allows scrapes to perform their initial scrape on component start
  and provide an initial delay. This means that scrapes will be delayed by `initial_delay`
  before first scrape and then run on `collection_interval` for each consecutive interval. 
  
- `batchprocessor`: Change multiBatcher to use sync.Map, avoid global lock on fast path (#7714)

### 🧰 Bug fixes 🧰

- `connectors`: When replicating data to connectors, consider whether the next pipeline will mutate data (#7776)

