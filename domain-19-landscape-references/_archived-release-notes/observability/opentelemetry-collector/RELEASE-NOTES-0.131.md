---
title: opentelemetry-collector v0.131 Release Notes
description: opentelemetry-collector v0.131 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.131 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- opa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.131 Release Notes 是什么
- 如何 opentelemetry-collector v0.131 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.131
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- policy-basics
- observability-basics
---



# opentelemetry-collector v0.131 Release Notes

Source: [v0.131.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.131.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.131.0

## End User Changelog

### 🛑 Breaking changes 🛑

- `confighttp`: Move `confighttp.framedSnappy` feature gate to beta. (#10584)

### 💡 Enhancements 💡

- `exporter/debug`: Move to alpha stability except profiles (#13487)
- `exporterhelper`: Enable `exporter.PersistRequestContext` feature gate by default. (#13437)
  Request context is now preserved by default when using persistent queues.
  Note that Auth extensions context is not propagated through the persistent queue.
  
- `pdata`: Use pdatagen to generate marshalJSON without using gogo proto jsonpb. (#13450)
- `otlpreceiver`: Remove usage of gogo proto which uses reflect.Value.MethodByName. Removes one source of disabling DCE. (#12747)
- `exporterhelper`: Fix metrics split logic to consider metrics description into the size. (#13418)
- `[[Service|service]]`: New pipeline instrumentation now differentiates internal failures from downstream errors (#13234)
  With the telemetry.newPipelineTelemetry feature gate enabled, the "received" and "produced"
  metrics related to a component now distinguish between two types of errors:
  - "outcome = failure" indicates that the component returned an internal error;
  - "outcome = refused" indicates that the component successfully emitted data, but returned an
    error coming from a downstream component processing that data.
  
- `pdata`: Remove usage of text/template from pdata, improves DCE. (#12747)
- `architecture`: New Tier 3 platform riscv64 allowing the collector to be built and distributed for this platform. (#13462)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Prevents the exporter for being stuck when telemetry data is bigger than batch.max_size (#12893)
- `mdatagen`: Fix import paths for mdatagen component (#13069)
- `otlpreceiver`: Error handler correctly fallbacks to content type (#13414)
- `pdata/pprofiles`: Fix profiles JSON unmarshal logic for originalPayload. The bytes have to be base64 encoded. (#13483)
- `xpdata`: Fix unmarshaling JSON for entities, add e2e tests to avoid this in the future. (#13480)
- `service`: Downgrade dependency of [[Prometheus|prometheus]] exporter in OTel Go SDK (#13429)
  This fixes the bug where collector's internal metrics are emitted with an unexpected suffix in their names when users configure the service::telemetry::metrics::readers with Prometheus
- `service`: Revert Default internal metrics config now enables `otel_scope_` labels (#12939, #13344)
  Reverting change temporarily due to prometheus exporter downgrade. This unfortunately re-introduces the bug that instrumentation scope attributes cause errors in Prometheus exporter. See http://github.com/open-telemetry/opentelemetry-collector/issues/12939 for details.
- `builder`: Remove undocumented handling of `DIST_*` environment variables replacements (#13335)

<!-- previous-version -->

## API Changelog

### 🛑 Breaking changes 🛑

- `configgrpc`: Update optional fields to use `configoptional.Optional` field for optional values. (#13252, #13364)
  Specifically, the following fields have been updated to `configoptional`:
  - `KeepaliveServerConfig.ServerParameters` (`KeepaliveServerParameters` type)
  - `KeepaliveServerConfig.EnforcementPolicy` (`KeepaliveEnforcementPolicy` type)
  
- `xexporterhelper`: Remove deprecated NewProfilesExporter function from xexporterhelper package (#13391)

### 💡 Enhancements 💡

- `consumererror`: Add new "Downstream" error marker (#13234)
  This new error wrapper type indicates that the error returned by a component's
  `Consume` method is not an internal failure of the component, but instead
  was passed through from another component further downstream.
  This is used internally by the new pipeline instrumentation feature to
  determine the `outcome` of a component call. This wrapper is not intended to
  be used by components directly.
  
- `pdata/pprofile`: Introduce `Equal` method on the `Function` type (#13222)
- `pdata/pprofile`: Introduce `Equal` method on the `Link` type (#13223)
- `pdata/pprofile`: Add new helper method `SetFunction` to set a new function on a line. (#13222)
- `pdata/pprofile`: Add new helper method `SetLink` to set a new link on a sample. (#13223)
- `pdata/pprofile`: Add new helper method `SetString` to set or retrieve the index of a value in the StringTable. (#13225)

<!-- previous-version -->
