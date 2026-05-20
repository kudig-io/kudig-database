---
title: opentelemetry-collector v0.92 Release Notes
description: opentelemetry-collector v0.92 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.92 Release Notes 是什么
- 如何 opentelemetry-collector v0.92 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.92
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.92 Release Notes

Source: [v0.92.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.92.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.92.0

# End user Changelog

## v1.0.1/v0.92.0

### 🛑 Breaking changes 🛑

- `exporters/sending_queue`: Do not re-enqueue failed batches, rely on the retry_on_failure strategy instead. (#8382)
  The current re-enqueuing behavior is not obvious and cannot be configured. It takes place only for persistent queue
  and only if `retry_on_failure::enabled=true` even if `retry_on_failure` is a setting for a different backoff retry
  strategy. This change removes the re-enqueuing behavior. Consider increasing `retry_on_failure::max_elapsed_time` 
  to reduce chances of data loss or set it to 0 to keep retrying until requests succeed.
  
- `confmap`: Make the option `WithErrorUnused` enabled by default when unmarshaling configuration (#7102)
  The option `WithErrorUnused` is now enabled by default, and a new option `WithIgnoreUnused` is introduced to ignore
  errors about unused fields.
  
- `status`: Deprecate `ReportComponentStatus` in favor of `ReportStatus`. This new function does not return an error. (#9148)

### 🚩 Deprecations 🚩

- `connectortest`: Deprecate connectortest.New[Metrics|Logs|Traces]Router in favour of connector.New[Metrics|Logs|Traces]Router (#9095)
- `exporterhelper`: Deprecate exporterhelper.RetrySettings in favor of configretry.BackOffConfig (#9091)
- `extension/ballast`: Deprecate `memory_ballast` extension. (#8343)
  Use `GOMEMLIMIT` environment variable instead.
  
- `connector`: Deprecate [Metrics|Logs|Traces]Router in favour of [Metrics|Logs|Traces]RouterAndConsumer (#9095)

### 💡 Enhancements 💡

- `exporterhelper`: Add RetrySettings validation function (#9089)
  Validate that time.Duration, multiplier values in configretry are non-negative, and randomization_factor is between 0 and 1
  
- `service`: Enable `telemetry.useOtelForInternalMetrics` by updating the flag to beta (#7454)
  The metrics generated should be consistent with the metrics generated
  previously with OpenCensus. Users can disable the behaviour
  by setting `--feature-gates -telemetry.useOtelForInternalMetrics` at
  collector start.
  
- `mdatagen`: move component from contrib to core (#9172)
- `semconv`: Generated Semantic conventions 1.22.0. (#8686)
- `confignet`: Add `dialer_timeout` config option. (#9066)
- `processor/memory_limiter`: Update config validation errors (#9059)
  - Fix names of the config fields that are validated in the error messages
  - Move the validation from start to the initialization phrase 
  
- `exporterhelper`: Add config Validate for TimeoutSettings (#9104)

### 🧰 Bug fixes 🧰

- `memorylimiterprocessor`: Fixed leaking goroutines from memorylimiterprocessor (#9099)
- `cmd/otelcorecol`: Fix the code detecting if the collector is running as a service on Windows. (#7350)
  Removed the `NO_WINDOWS_SERVICE` environment variable given it is not needed anymore.
- `otlpexporter`: remove dependency of otlphttpreceiver on otlpexporter (#6454)

# API Changelog

This changelog includes only developer-facing changes.
If you are looking for user-facing changes, check out [CHANGELOG.md](./CHANGELOG.md).

<!-- next version -->

## v1.0.1/v0.92.0

### 🛑 Breaking changes 🛑

- `otlpexporter`: Change Config members names to use Config suffix. (#9091)
- `component`: Remove deprecated unused TelemetrySettingsBase (#9145)

### 🚩 Deprecations 🚩

- `confignet`: Deprecates the `Dial` and `Listen` functions in favor of `DialContext` and `ListenContext`. (#9163)
- `component`: Deprecate unnecessary type StatusFunc (#9146)