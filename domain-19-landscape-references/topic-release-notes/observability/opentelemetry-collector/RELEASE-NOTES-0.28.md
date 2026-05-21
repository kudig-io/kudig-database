---
title: opentelemetry-collector v0.28 Release Notes
description: opentelemetry-collector v0.28 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.28 Release Notes 是什么
- 如何 opentelemetry-collector v0.28 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- observability-basics
---

# opentelemetry-collector v0.28 Release Notes

Source: [v0.28.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.28.0)

# v0.28.0 Beta

## 🛑 Breaking changes 🛑

- Remove unused logstest package (#3222)
- Introduce `AppSettings` instead of `Parameters` (#3163)
- Remove unused testutil.TempSocketName (#3291)
- Move BigEndian helper functions in `tracetranslator` to an internal package.(#3298)
- Rename `configtest.LoadConfigFile` to `configtest.LoadConfigAndValidate` (#3306)
- Replace `ExtensionCreateParams` with `ExtensionCreateSettings` (#3294)
- Replace `ProcessorCreateParams` with `ProcessorCreateSettings`. (#3181)
- Replace `ExporterCreateParams` with `ExporterCreateSettings` (#3164)
- Replace `ReceiverCreateParams` with `ReceiverCreateSettings`. (#3167)
- Change `batchprocessor` logic to limit data points rather than metrics (#3141)
- Rename `PrwExporter` to `PRWExporter` and `NewPrwExporter` to `NewPRWExporter` (#3246)
- Avoid exposing OpenCensus reference in public APIs (#3253)
- Move `config.Parser` to `configparser.Parser` (#3304)
- Remove deprecated funcs inside the obsreceiver (#3314)
- Remove `obsreport.GRPCServerWithObservabilityEnabled`, enable observability in config (#3315)
- Remove `obsreport.ProcessorMetricViews`, use `BuildProcessorCustomMetricName` where needed (#3316)
- Remove "Receive" from `obsreport.Receiver` funcs (#3326)
- Remove "Export" from `obsreport.Exporter` funcs (#3333)
- Hide unnecessary public struct `obsreport.StartReceiveOptions` (#3353)
- Avoid exposing internal implementation public in OC/OTEL receivers (#3355)
- Updated configgrpc `ToDialOptions` and confighttp `ToClient` apis to take extensions configuration map (#3340)
- Remove `GenerateSequentialTraceID` and `GenerateSequentialSpanIDin` functions in testbed (#3390)
- Change "grpc" to "GRPC" in configauth function/type names (#3285)

## 💡 Enhancements 💡

- Add `doc.go` files to the consumer package and its subpackages (#3270)
- Automate triggering of doc-update on release (#3234)
- Enable Dependabot for Github Actions (#3312)
- Remove the proto dependency in `goldendataset` for traces (#3322)
- Add telemetry for dropped data due to exporter sending queue overflow (#3328)
- Add initial implementation of `pdatagrcp` (#3231)
- Change receiver obsreport helpers pattern to match the Processor/Exporter (#3227)
- Add model translation and encoding interfaces (#3200)
- Add otlpjson as a serializer implementation (#3238)
- `prometheus` receiver:
  - Add `createNodeAndResourcePdata` for Prometheus->OTLP pdata (#3139)
  - Direct metricfamily Prometheus->OTLP (#3145)
- Add `componenttest.NewNop*CreateSettings` to simplify tests (#3375)
- Add support for markdown generation (#3100)
- Refactor components for the Client Authentication Extensions (#3287)

## 🧰 Bug fixes 🧰

- Use dedicated `zapcore.Core` for Windows service (#3147)
- Hook up start and shutdown functions in fileexporter (#3260)
- Fix oc to pdata translation for sum non-monotonic cumulative (#3272)
- Fix `timeseriesSignature` in prometheus receiver (#3310)
