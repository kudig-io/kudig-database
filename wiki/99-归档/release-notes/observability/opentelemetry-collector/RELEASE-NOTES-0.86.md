---
title: opentelemetry-collector v0.86 Release Notes
description: opentelemetry-collector v0.86 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.86 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- opentelemetry-collector v0.86 Release Notes 是什么
- 如何 opentelemetry-collector v0.86 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.86
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# opentelemetry-collector v0.86 Release Notes

Source: [v0.86.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.86.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.86.0

## User facing changes

### 🚩 Deprecations 🚩

- `loggingexporter`: Mark the logging exporter as deprecated, in favour of debug exporter (#7769)

### 🚀 New components 🚀

- `debugexporter`: Add debug exporter, which replaces the logging exporter (#7769)

### 💡 Enhancements 💡

- `featuregate`: List valid feature gates when failing to load invalid gate (#8505)
- `supported platforms`: Add `linux/s390x` architecture to cross build tests in CI (#8213)

### 🧰 Bug fixes 🧰

- `builder`: fix setting `dist.*` keys from env (#8239)
- `configtls`: fix incorrect use of fsnotify (#8438)

## API changes

### 🛑 Breaking changes 🛑

- `[[Service|service]]`: remove deprecated service.PipelineConfig (#8485)

### 🚩 Deprecations 🚩

- `obsreporttest`: deprecate To*CreateSettings funcs in obsreporttest (#8492)
  The following TestTelemetry methods have been deprecated. Use structs instead:
  -  ToExporterCreateSettings -> exporter.CreateSettings
  -  ToProcessorCreateSettings -> processor.CreateSettings
  -  ToReceiverCreateSettings -> receiver.CreateSettings
  
- `obsreport`: Deprecating `obsreport.Exporter`, `obsreport.ExporterSettings`, `obsreport.NewExporter` (#8492)
  These deprecated methods/structs have been moved to exporterhelper:
  - `obsreport.Exporter` -> `exporterhelper.ObsReport`
  - `obsreport.ExporterSettings` -> `exporterhelper.ObsReportSettings`
  - `obsreport.NewExporter` -> `exporterhelper.NewObsReport`
  
- `obsreport`: Deprecating `obsreport.BuildProcessorCustomMetricName`, `obsreport.Processor`, `obsreport.ProcessorSettings`, `obsreport.NewProcessor` (#8492)
  These deprecated methods/structs have been moved to processorhelper:
  - `obsreport.BuildProcessorCustomMetricName` -> `processorhelper.BuildCustomMetricName`
  - `obsreport.Processor` -> `processorhelper.ObsReport`
  - `obsreport.ProcessorSettings` -> `processorhelper.ObsReportSettings`
  - `obsreport.NewProcessor` -> `processorhelper.NewObsReport`
  
- `obsreport`: Deprecating obsreport scraper and receiver API (#8492)
  These deprecated methods/structs have been moved to receiverhelper and scraperhelper:
  - `obsreport.Receiver` -> `receiverhelper.ObsReport`
  - `obsreport.ReceiverSettings` -> `receiverhelper.ObsReportSettings`
  - `obsreport.NewReceiver` -> `receiverhelper.NewObsReport`
  - `obsreport.Scraper` -> `scraperhelper.ObsReport`
  - `obsreport.ScraperSettings` -> `scraperhelper.ObsReportSettings`
  - `obsreport.NewScraper` -> `scraperhelper.NewObsReport`
  

### 💡 Enhancements 💡

- `otelcol`: Splitting otelcol into its own module. (#7924)
- `service`: Split service into its own module (#7923)

<!-- risk-assessed -->
