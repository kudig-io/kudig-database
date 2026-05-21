---
title: opentelemetry-collector v0.145 Release Notes
description: opentelemetry-collector v0.145 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.145 Release Notes 是什么
- 如何 opentelemetry-collector v0.145 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.145
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

# opentelemetry-collector v0.145 Release Notes

Source: [v0.145.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.145.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.145.0

## End User Changelog

### 💡 Enhancements 💡

- `pkg/scraperhelper`: ScraperID has been added to the logs for metrics, logs, and profiles (#14461)

### 🧰 Bug fixes 🧰

- `exporter/otlp_grpc`: Fix the OTLP exporter balancer to use round_robin by default, as intended. (#14090)
- `pkg/config/configoptional`: Fix `Unmarshal` methods not being called when config is wrapped inside `Optional` (#14500)
  This bug notably manifested in the fact that the `sending_queue::batch::sizer` config for exporters
  stopped defaulting to `sending_queue::sizer`, which sometimes caused the wrong units to be used
  when configuring `sending_queue::batch::min_size` and `max_size`.
  
  As part of the fix, `xconfmap` exposes a new `xconfmap.WithForceUnmarshaler` option, to be used in the `Unmarshal` methods
  of wrapper types like `configoptional.Optional` to make sure the `Unmarshal` method of the inner type is called.
  
  The default behavior remains that calling `conf.Unmarshal` on the `confmap.Conf` passed as argument to an `Unmarshal`
  method will skip any top-level `Unmarshal` methods to avoid infinite recursion in standard use cases. 
  
- `pkg/confmap`: Fix an issue where configs could fail to decode when using interpolated values in string fields. (#14413)
  For example, a header can be set via an environment variable to a string that is parseable as a number, e.g. `1234`
  
- `pkg/service`: Don't error on startup when process metrics are enabled on unsupported OSes (e.g. AIX) (#14307)

<!-- previous-version -->

## API Changelog

### 💡 Enhancements 💡

- `pkg/config/configgrpc`: add client info to context before server authentication (#12836)
- `pkg/xscraperhelper`: Add AddProfilesScraper similar to scraperhelper.AddMetricsScraper (#14427)

### 🧰 Bug fixes 🧰

- `pkg/config/configoptional`: Fix `Unmarshal` methods not being called when config is wrapped inside `Optional` (#14500)
  This bug notably manifested in the fact that the `sending_queue::batch::sizer` config for exporters
  stopped defaulting to `sending_queue::sizer`, which sometimes caused the wrong units to be used
  when configuring `sending_queue::batch::min_size` and `max_size`.
  
  As part of the fix, `xconfmap` exposes a new `xconfmap.WithForceUnmarshaler` option, to be used in the `Unmarshal` methods
  of wrapper types like `configoptional.Optional` to make sure the `Unmarshal` method of the inner type is called.
  
  The default behavior remains that calling `conf.Unmarshal` on the `confmap.Conf` passed as argument to an `Unmarshal`
  method will skip any top-level `Unmarshal` methods to avoid infinite recursion in standard use cases. 
  

<!-- previous-version -->
