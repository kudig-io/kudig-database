# opentelemetry-collector v0.13 Release Notes

Source: [v0.13.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.13.0)

# v0.13.0 Beta

## 🛑 Breaking changes 🛑

- Host metric `system.disk.time` renamed to `system.disk.operation_time` (#1887)
- Use consumer for sender interface, remove unnecessary receiver address from Runner (#1941)
- Enable sending queue by default in all exporters configured to use it (#1924)
- Removed `groupbytraceprocessor` (#1891)
- Remove ability to configure collection interval per scraper (#1947)

## 💡 Enhancements 💡

- Host Metrics receiver now reports both `system.disk.io_time` and `system.disk.operation_time` (#1887)
- Match spans against the instrumentation library and resource attributes (#928)
- Add `receiverhelper` for creating flexible "scraper" metrics receiver (#1886, #1890, #1945, #1946)
- Migrate `tailsampling` processor to new OTLP-based internal data model and add Composite Sampler (#1894)
- Metadata Generator: Change Metrics fields to implement an interface with new methods (#1912)
- Add unmarshalling for `pdata.Traces` (#1948)
- Add debug-level message on error for `jaeger` exporter (#1964)

## 🧰 Bug fixes 🧰

- Fix bug where the service does not correctly start/stop the log exporters (#1943)
- Fix Queued Retry Unusable without Batch Processor (#1813) - (#1930)
- `prometheus` receiver: Log error message when `process_start_time_seconds` gauge is missing (#1921)
- Fix trace jaeger conversion to internal traces zero time bug (#1957)
- Fix panic in otlp traces to zipkin (#1963)
- Fix OTLP/HTTP receiver's path to be /v1/traces (#1979)