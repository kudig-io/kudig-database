---
title: opentelemetry-collector v0.29 Release Notes
description: opentelemetry-collector v0.29 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.29 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- kafka
- gateway
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.29 Release Notes 是什么
- 如何 opentelemetry-collector v0.29 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.29
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- kafka-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.29 Release Notes

Source: [v0.29.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.29.0)

## v0.29.0 Beta

## 🛑 Breaking changes 🛑

- Rename `[[Service|service]].Application` to `service.Collector` (#3268)
- Provide case sensitivity in config yaml mappings by using Koanf instead of Viper (#3337)
- Move zipkin constants to an internal package (#3431)
- Disallow renaming metrics using metric relabel configs (#3410)
- Move cgroup and iruntime utils from memory_limiter to internal folder (#3448)
- Move model pdata interfaces to pdata, expose them publicly (#3455)

## 💡 Enhancements 💡

- Change obsreport helpers for scraper to use the same pattern as Processor/Exporter (#3327)
- Convert `otlptext` to implement Marshaler interfaces (#3366)
- Add encoder/decoder and marshaler/unmarshaler for OTLP protobuf (#3401)
- Use the new marshaler/unmarshaler in `kafka` exporter (#3403)
- Convert `zipkinv2` to to/from translator interfaces (#3409)
- `zipkinv1`: Move to translator and encoders interfaces (#3419)
- Use the new marshaler/unmarshaler in `kafka` receiver #3402
- Change `oltp` receiver to use the new unmarshaler, avoid grpc-gateway dependency (#3406)
- Use the new Marshaler in the `otlphttp` exporter (#3433)
- Add [[gRPC|grpc]] response struct for all signals instead of returning interface in `otlp` receiver/exporter (#3437)
- `zipkinv2`: Add encoders, decoders, marshalers (#3426)
- `scrapererror` receiver: Return concrete error type (#3360)
- `kafka` receiver: Add metrics support (#3452)
- `[[Prometheus|prometheus]]` receiver:
  - Add store to track stale metrics (#3414)
  - Add `up` and `scrape_xxxx` internal metrics (#3116)

## 🧰 Bug fixes 🧰

- `prometheus` receiver:
  - Reject datapoints with duplicate label keys (#3408)
  - Scrapers are not stopped when receiver is shutdown (#3450)
- `prometheusremotewrite` exporter: Adjust default retry settings (#3416)
- `hostmetrics` receiver: Fix missing startTimestamp for `processes` scraper (#3461)


<!-- risk-assessed -->
