# opentelemetry-collector v0.19 Release Notes

Source: [v0.19.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.19.0)

# v0.19.0 Beta

## 🛑 Breaking changes 🛑
- Remove deprecated `queued_retry` processor
- Remove deprecated configs from `resource` processor: `type` (set "opencensus.type" key in "attributes.upsert" map instead) and `labels` (use "attributes.upsert" instead).

## 💡 Enhancements 💡

- `hostmetrics` receiver: Refactor load metrics to use generated metrics (#2375)
- Add uptime to the servicez debug page (#2385)
- Add new semantic conventions for AWS (#2365)

## 🧰 Bug fixes 🧰

- `jaeger` exporter: Improve connection state logging (#2239)
- `pdatagen`: Fix slice of values generated code (#2403)
- `filterset` processor: Avoid returning always nil error in strict filterset (#2399)