# opentelemetry-collector v0.18 Release Notes

Source: [v0.18.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.18.0)

# v0.18.0 Beta

## 🛑 Breaking changes 🛑

- Rename host metrics according to metrics spec and rename `swap` scraper to `paging` (#2311)

## 💡 Enhancements 💡

- Add check for `NO_WINDOWS_SERVICE` environment variable to force interactive mode on Windows (#2272)
- `hostmetrics` receiver: Add `disk/weighted_io_time` metric (Linux only) (#2312)
- `opencensus` exporter: Add queue-retry (#2307)
- `filter` processor: Filter metrics using resource attributes (#2251)

## 🧰 Bug fixes 🧰

- `fluentforward` receiver: Fix string conversions (#2314)
- Fix zipkinv2 translation error tag handling (#2253)
