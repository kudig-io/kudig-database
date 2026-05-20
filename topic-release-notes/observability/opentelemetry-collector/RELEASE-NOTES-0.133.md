---
title: opentelemetry-collector v0.133 Release Notes
description: opentelemetry-collector v0.133 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.133 Release Notes 是什么
- 如何 opentelemetry-collector v0.133 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.133
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.133 Release Notes

Source: [v0.133.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.133.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.133.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `all`: Increase minimum Go version to 1.24 (#13627)

### 💡 Enhancements 💡

- `otlphttpexporter`: Add `profiles_endpoint` configuration option to allow custom endpoint for profiles data export (#13504)
  The `profiles_endpoint` configuration follows the same pattern as `traces_endpoint`, `metrics_endpoint`, and `logs_endpoint`.
  When specified, profiles data will be sent to the custom URL instead of the default `{endpoint}/v1development/profiles`.
  
- `pdata`: Add support for local memory pooling for data objects. (#13678)
  This is still an early experimental (alpha) feature. Do not recommended to be used production. To enable use "--featuregate=+pdata.useProtoPooling"
- `pdata`: Optimize CopyTo messages to avoid any copy when same source and destination (#13680)
- `receiverhelper`: New feature flag to make receiverhelper distinguish internal vs. downstream errors using new `otelcol_receiver_failed_x` and `otelcol_receiver_requests` metrics (#12207, #12802)
  This is a breaking change for the semantics of the otelcol_receiver_refused_metric_points,  otelcol_receiver_refused_log_records and otelcol_receiver_refused_spans metrics.
  These new metrics and semantics are enabled through the `receiverhelper.newReceiverMetrics` feature gate.
  
- `debugexporter`: Add support for entity references in debug exporter output (#13324)
- `pdata`: Fix unnecessary allocation of a new state when adding new values to pcommon.Map (#13634)
- `service`: Implement refcounting for pipeline data owned memory. (#13631)
  This feature is protected by `--featuregate=+pdata.useProtoPooling`.
- `service`: Add a debug-level log message when a consumer returns an error. (#13357)
- `xpdata`: Optimize xpdata/context for persistent queue when only one value for key (#13636)
- `otlpreceiver`: Log the listening addresses of the receiver, rather than the configured endpoints. (#13654)
- `pdata`: Use the newly added proto marshaler/unmarshaler for the official proto Marshaler/Unmarshaler (#13637)
  If any problems observed with this consider to disable the featuregate `--feature-gates=-pdata.useCustomProtoEncoding`
<!-- cspell:ignore MLKEM mlkem -->
- `configtls`: Enable X25519MLKEM768 as per draft-ietf-tls-ecdhe-mlkem (#13670)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Prevent uncontrolled goroutines in batcher due to a incorrect worker pool behaviour. (#13689)
- `service`: Ensure the insecure configuration is accounted for when normalizing the endpoint. (#13691)
- `configoptional`: Allow validating nested types (#13579)
  `configoptional.Optional` now implements `xconfmap.Validator`
- `batchprocessor`: Fix UB in batch processor when trying to read bytes size after adding request to pipeline (#13698)
  This bug only happens id detailed metrics are enabled and also an async (sending queue enabled) exporter that mutates data is configure.

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `configgrpc`: Set `tcp` as the default transport type (#13657)
  gRPC is generally used with HTTP/2, so this will simplify usage for most components.

### 🚩 Deprecations 🚩

- `pdata/pprofile`: Deprecate Profiles.ProfilesDictionary in favor of Profiles.Dictionary. (#13644)

### 💡 Enhancements 💡

- `pdata`: Add support for local memory pooling for data objects. (#13678)
  This is still an early experimental (alpha) feature. Do not recommended to be used production. To enable use "--featuregate=+pdata.useProtoPooling"

### 🧰 Bug fixes 🧰

- `configoptional`: Allow validating nested types (#13579)
  `configoptional.Optional` now implements `xconfmap.Validator`

<!-- previous-version -->
