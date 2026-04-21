# opentelemetry-collector v0.4 Release Notes

Source: [v0.4.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.4.0)

# 🎉 OpenTelemetry Collector v0.4.0 (Beta) 🎉

The OpenTelemetry Collector offers a vendor-agnostic implementation on how to receive, process, and export telemetry data. Check out the [Getting Started Guide](https://opentelemetry.io/docs/collector/about/) for deployment and configuration information.

For community-based components including support for commercial back-ends, please check out the [opentelemetry-collector-contrib release](https://github.com/open-telemetry/opentelemetry-collector-contrib/releases).

## 🛑 Breaking changes 🛑

- `isEnabled` configuration option removed (#909) 
- `thrift_tchannel` protocol moved from `jaeger` receiver to `jaeger_legacy` in contrib (#636) 

## ⚠️ Major changes ⚠️

- Switch from `localhost` to `0.0.0.0` by default for all receivers (#1006)
- Internal API Changes (only impacts contributors)
  - Add context to `Start` and `Stop` methods in the component (#790)
  - Rename `AttributeValue` and `AttributeMap` method names (#781)
(other breaking changes in the internal trace data types)
  - Change entire repo to use the new vanityurl go.opentelemetry.io/collector (#977)

## 🚀 New components 🚀

- Receivers
  - `hostmetrics` receiver with CPU (#862), disk (#921), load (#974), filesystem (#926), memory (#911), network (#930), and virtual memory (#989) support
- Processors
  - `batch` for batching received metrics (#1060) 
  - `filter` for filtering (dropping) received metrics (#1001) 

## 💡 Enhancements 💡

- `otlp` receiver implement HTTP X-Protobuf (#1021)
- Exporters: Support mTLS in gRPC exporters (#927) 
- Extensions: Add `zpages` for service (servicez, pipelinez, extensions) (#894) 

## 🧰 Bug fixes 🧰

- Add missing logging for metrics at `debug` level (#1108) 
- Fix setting internal status code in `jaeger` receivers (#1105) 
- `zipkin` export fails on span without timestamp when used with `queued_retry` (#1068) 
- Fix `zipkin` receiver status code conversion (#996) 
- Remove extra send/receive annotations with using `zipkin` v1 (#960)
- Fix resource attribute mutation bug when exporting in `jaeger` proto (#907) 
- Fix metric/spans count, add tests for nil entries in the slices (#787) 

## 📦 Release bits 📦

- Docker: `docker pull otel/opentelemetry-collector:0.4.0`
- Binary checksums: Generated with `shasum -a 256 bin/*` and are present in the checksums.txt file

## 🧩 Components 🧩

### Traces

| Receivers | Processors | Exporters |
|:----------:|:-----------:|:----------:|
| Jaeger | Attributes | File |
| OpenCensus | Batch | Jaeger |
| OTLP | Memory Limiter | Logging |
| Zipkin | Queued Retry | OpenCensus |
| | Resource | OTLP |
| | Sampling | Zipkin |
| | Span ||

### Metrics

| Receivers | Processors | Exporters |
|:----------:|:-----------:|:----------:|
| HostMetrics | Batch | File |
| OpenCensus | Filter | Logging |
| OTLP | Memory Limiter | OpenCensus |
| Prometheus || OTLP |
| VM Metrics || Prometheus |

### Extensions

- Health Check
- Performance Profiler
- zPages