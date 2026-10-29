---
title: opentelemetry-collector v0.116 Release Notes
description: opentelemetry-collector v0.116 Release Notes — Kubernetes 生产运维知识库
summary: opentelemetry-collector v0.116 Release Notes — Kubernetes 生产运维知识库
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
- opentelemetry-collector v0.116 Release Notes 是什么
- 如何 opentelemetry-collector v0.116 Release Notes
trigger_keywords:
- opentelemetry-collector
- v0.116
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




# opentelemetry-collector v0.116 Release Notes

Source: [v0.116.0](https://github.com/open-telemetry/opentelemetry-collector/releases/tag/v0.116.0)

### Images and binaries here: https://github.com/open-telemetry/opentelemetry-collector-releases/releases/tag/v0.116.0

## End User Changelog

## v1.22.0/v0.116.0

### 🛑 Breaking changes 🛑

- `pdata/pprofile`: Remove deprecated `Profile.EndTime` and `Profile.SetEndTime` methods. (#11796)

### 💡 Enhancements 💡

- `xconfighttp`: Add WithOtelHTTPOptions to experimental module xconfighttp (#11770)

### 🧰 Bug fixes 🧰

- `exporterhelper`: Fix memory leak at exporter shutdown (#11401)
- `sharedcomponent`: Remove race-condition and cleanup locking (#11819)

## API Changelog

## v1.22.0/v0.116.0

### 🛑 Breaking changes 🛑

- `component`: Remove deprecated TelemetrySettings.LeveledMeterProvider (#11811)
- `scraperhelper`: Remove deprecated scraperhelper.Scraper and helpers (#11803)

### 🚩 Deprecations 🚩

- `connector`: Deprecate connectorprofiles module in favor of xconnector to allow adding more experimental data types. (#11778)
- `consumererror`: Deprecate consumererrorprofiles module in favor of xconsumererror to allow adding more experimental data types. (#11778)
- `consumer`: Deprecate consumerprofiles module in favor of xconsumer to allow adding more experimental data types. (#11778)
- `exporterhelper`: Deprecate exporterhelperprofiles module in favor of xexporterhelper to allow adding more experimental data types. (#11778)
- `exporter`: Deprecate exporterprofiles module in favor of xexporter to allow adding more experimental data types. (#11778)
- `pipeline`: Deprecate pipelineprofiles module in favor of xpipeline to allow adding more experimental data types. (#11778)
- `processorhelper`: Deprecate processorhelperprofiles module in favor of xprocessorhelper to allow adding more experimental data types. (#11778)
- `processor`: Deprecate processorprofiles module in favor of xprocessor to allow adding more experimental data types. (#11778)
- `receiver`: Deprecate receiverprofiles module in favor of xreceiver to allow adding more experimental data types. (#11778)
- `receiver/scrapererror`: Remove the receiver/scrapererror alias. (#11003)

### 💡 Enhancements 💡

- `receiver/scraperhelper`: Add scraper for logs (#11238)

<!-- risk-assessed -->
