# opentelemetry-collector v0.3 Release Notes

Source: [v0.3.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.3.0)

## Docker images:

* Core: `docker pull otel/opentelemetry-collector:0.3.0`
  * Supported open-source receivers and exporters
  * Supported processors and extensions
* [Contrib](https://github.com/open-telemetry/opentelemetry-collector-contrib): `docker pull otel/opentelemetry-collector-contrib:0.3.0`
  * Community open-source receivers and exporters including for commercial back-ends
  * Community processors and extensions

## Breaking changes

-  Make prometheus reciever config loading strict. #697 
Prometheus receiver will now fail fast if the config contains unused keys in it.

## Changes and fixes

- Enable best effort serve by default of Prometheus Exporter (https://github.com/orijtech/prometheus-go-metrics-exporter/pull/6)
- Fix null pointer exception in the logging exporter #743 
- Remove unnecessary condition to have at least one processor #744 

## Components

| Receivers / Exporters | Processors | Extensions |
|:---------------------:|:-----------:|:-----------:|
| Jaeger | Attributes | Health Check |
| OpenCensus | Batch | Performance Profiler |
| OpenTelemetry | Memory Limiter | zPages |
| Zipkin | Queued Retry | |
| | Resource | |
| | Sampling | |
| | Span | |


## Checksums

Checksums were generated with `shasum -a 256 bin/*` and are present in the checksums.txt file. 