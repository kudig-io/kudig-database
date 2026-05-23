---
title: opentelemetry-collector v0.147 Release Notes
description: opentelemetry-collector v0.147 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.147 Release Notes 是什么
- 如何 opentelemetry-collector v0.147 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.147
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
created: "2026-05-23"
---

# opentelemetry-collector v0.147 Release Notes

Source: [v0.147.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.147.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.147.0

## End User Changelog

### 💡 Enhancements 💡

- `exporter/debug`: Output bucket counts for exponential histogram data points in normal verbosity. (#10463)
- `pkg/exporterhelper`: Add `metadata_keys` configuration to `sending_queue.batch.partition` to partition batches by client metadata (#14139)
  The `metadata_keys` configuration option is now available in the `sending_queue.batch.partition` section for all exporters.
  When specified, batches are partitioned based on the values of the listed metadata keys, allowing separate batching per metadata partition. This feature
  is automatically configured when using `exporterhelper.WithQueue()`.
  

### 🧰 Bug fixes 🧰

- `cmd/builder`: Fix duplicate error output when CLI command execution fails in the builder tool. (#14436)
- `cmd/mdatagen`: Fix duplicate error output when CLI command execution fails in the mdatagen tool. (#14436)
- `cmd/mdatagen`: Fix semconv URL validation for metrics with underscores in their names (#14583)
  Metrics like `system.disk.io_time` now correctly validate against semantic convention URLs containing underscores in the anchor tag.
- `extension/memory_limiter`: Use ChainUnaryInterceptor instead of UnaryInterceptor to allow multiple interceptors. (#14634)
  If multiple extensions that use the UnaryInterceptor are set the binary panics at start time.
- `extension/memory_limiter`: Add support for streaming services. (#14634)
- `pkg/config/configmiddleware`: Add context.Context to HTTP middleware interface constructors. (#14523)
  This is a breaking API change for components that implement or use extensionmiddleware.
- `pkg/confmap`: Fix another issue where configs could fail to decode when using interpolated values in string fields. (#14034)
  For example, a resource attribute can be set via an environment variable to a string that is parseable as a number, e.g. `1234`.
  
  (A similar bug was fixed in a previous release: that one was triggered when the field was nested in a struct,
  whereas this one is triggered when the field internally has type "pointer to string" rather than "string".)
  
- `pkg/otelcol`: The featuregate subcommand now rejects extra positional arguments instead of silently ignoring them. (#14554)
- `pkg/queuebatch`: Fix data race in partition_batcher where resetTimer() was called outside mutex, causing concurrent timer.Reset() calls and unpredictable batch flush timing under load. (#14491)
- `pkg/scraperhelper`: Log scrapers now emit log-appropriate receiver telemetry (#14654)
  Log scrapers previously emitted the same receiver telemetry as metric scrapers,
  such as the otelcol_receiver_accepted_metric_points metric (instead of otelcol_receiver_accepted_log_records),
  or spans named receiver/myreceiver/MetricsReceived (instead of receiver/myreceiver/LogsReceived).
  
  This did not affect scraper-specific spans and metrics.
  
- `processor/batch`: Fixes a bug where the batch processor would not copy `SchemaUrl` metadata from resource and scope containers during partial batch splits. (#12279, #14620)

<!-- previous-version -->

## API Changelog

### 💡 Enhancements 💡

- `pkg/exporterhelper`: Add `metadata_keys` configuration to `sending_queue.batch.partition` to partition batches by client metadata (#14139)
  The `metadata_keys` configuration option is now available in the `sending_queue.batch.partition` section for all exporters.
  When specified, batches are partitioned based on the values of the listed metadata keys, allowing separate batching per metadata partition. This feature
  is automatically configured when using `exporterhelper.WithQueue()`.
  
- `pkg/xexporterhelper`: Add code structure to handle unbounded partitions in sending queue. (#14526)

### 🧰 Bug fixes 🧰

- `pkg/config/configmiddleware`: Add context.Context to gRPC middleware interface constructors. (#14523)
- `pkg/extensionmiddleware`: Add context.Context to gRPC middleware interface constructors. (#14523)
  This is a breaking API change for components that implement or use extensionmiddleware.

<!-- previous-version -->
