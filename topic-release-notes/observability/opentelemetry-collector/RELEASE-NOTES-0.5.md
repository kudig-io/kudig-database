---
title: opentelemetry-collector v0.5 Release Notes
description: opentelemetry-collector v0.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- docker
- job
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.5 Release Notes 是什么
- 如何 opentelemetry-collector v0.5 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.5
- Release
- Notes
- release
- notes
---

# opentelemetry-collector v0.5 Release Notes

Source: [v0.5.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.5.0)

# 🎉 OpenTelemetry Collector v0.5.0 (Beta) 🎉

## 🛑 Breaking changes 🛑

- **Update OTLP to v0.4.0 (#1142)**: Collector will be incompatible with any other sender or receiver of OTLP protocol
of different versions
- Make "--new-metrics" command line flag the default (#1148)
- Change `endpoint` to `url` in Zipkin exporter config (#1186)
- Change `tls_credentials` to `tls_settings` in Jaegar receiver config (#1233)
- OTLP receiver config change for `protocols` to support mTLS (#1223)
- Remove `export_resource_labels` flag from Zipkin exporter (#1163)

## 🚀 New components 🚀

- Receivers
  - Added process scraper to the `hostmetrics` receiver (#1047)

## 💡 Enhancements 💡

- otlpexporter: send configured headers in request (#1130)
- Enable Collector to be run as a Windows service (#1120)
- Add config for HttpServer (#1196)
- Allow cors in HTTPServerSettings (#1211)
- Add a generic grpc server settings config, cleanup client config (#1183)
- Rely on gRPC to batch and loadbalance between connections instead of custom logic (#1212)
- Allow to tune the read/write buffers for gRPC clients (#1213)
- Allow to tune the read/write buffers for gRPC server (#1218)

## 🧰 Bug fixes 🧰

- Handle overlapping metrics from different jobs in prometheus exporter (#1096)
- Fix handling of SpanKind INTERNAL in OTLP OC translation (#1143)
- Unify zipkin v1 and v2 annotation/tag parsing logic (#1002)
- mTLS: Add support to configure client CA and enforce ClientAuth (#1185)
- Fixed untyped Prometheus receiver bug (#1194)
- Do not embed ProtocolServerSettings in gRPC (#1210)
- Add Context to the missing CreateMetricsReceiver method (#1216)

📦 Release bits 📦

    Docker: docker pull otel/opentelemetry-collector:0.4.1
    Binary checksums: Generated with shasum -a 256 bin/* and are present in the checksums.txt file
