# opentelemetry-collector v0.15 Release Notes

Source: [v0.15.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.15.0)

## v0.15.0 Beta

## 🛑 Breaking changes 🛑

- Remove legacy metrics, they were marked as legacy for ~12 months #2105

## 💡 Enhancements 💡

- Implement conversion between OpenCensus and OpenTelemetry Summary Metric (#2048)
- Add ability to generate non nullable messages (#2005)
- Implement Summary Metric in Prometheus RemoteWrite Exporter (#2083)
- Add `resource_to_telemetry_conversion` to exporter helper expose exporter settings (#2060)
- Add `CustomRoundTripper` function to httpclientconfig (#2085)
- Allow for more logging options to be passed to `service` (#2132)
- Add config parameters for `jaeger` receiver (#2068)
- Map unset status code for `jaegar` translator as per spec (#2134)
- Add more trace annotations to the queue-retry logic (#2136)
- Add config settings for component telemetry (#2148)
- Use net.SplitHostPort for IPv6 support in `prometheus` receiver (#2154)
- Add --log-format command line option (default to "console") #2177.

## 🧰 Bug fixes 🧰

- `logging` exporter: Add Logging for Summary Datapoint (#2084)
- `hostmetrics` receiver: use correct TCP state labels on Unix systems (#2087)
- Fix otlp_log receiver wrong use of trace measurement (#2117)
- Fix "process/memory/rss" metric units (#2112)
- Fix "process/cpu_seconds" metrics (#2113)
- Add check for nil logger in exporterhelper functions (#2141)
- `prometheus` receiver:
  - Upgrade Prometheus version to fix race condition (#2121)
  - Fix the scraper/discover manager coordination (#2089)
  - Fix panic when adjusting buckets (#2168)