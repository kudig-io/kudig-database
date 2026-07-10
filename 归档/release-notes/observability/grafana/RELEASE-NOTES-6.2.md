---
title: grafana v6.2 Release Notes
description: grafana v6.2 Release Notes — Kubernetes 生产运维知识库
summary: grafana v6.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- elasticsearch
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- grafana v6.2 Release Notes 是什么
- 如何 grafana v6.2 Release Notes
trigger_keywords:
- grafana
- v6.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# grafana v6.2 Release Notes

Source: [v6.2.5](https://github.com/grafana/grafana/releases/tag/v6.2.5)

[Download Page](https://grafana.com/grafana/download)
[What's New Highlights](https://grafana.com/docs/guides/whats-new-in-v6-2/)
[Release Notes](https://community.grafana.com/t/release-notes-v6-2-x/17037)

# 6.2.5 (2019-05-25)

  ### Features / Enhancements
  * **Grafana-CLI**: Wrapper for `grafana-cli` within RPM/DEB packages and config/homepath are now global flags. [#17695](https://github.com/grafana/grafana/pull/17695), [@gotjosh](https://github.com/gotjosh)
  * **Panel**: Fully escape html in drilldown links (was only sanitized before) . [#17731](https://github.com/grafana/grafana/pull/17731), [@dehrax](https://github.com/dehrax)
  
  ### Bug Fixes
  * **Config**: Fix connectionstring for remote_cache in defaults.ini. [#17675](https://github.com/grafana/grafana/pull/17675), [@kylebrandt](https://github.com/kylebrandt)
  * **Elasticsearch**: Fix empty query (via template variable) should be sent as wildcard. [#17488](https://github.com/grafana/grafana/pull/17488), [@davewat](https://github.com/davewat)
  * **HTTP-Server**: Fix Strict-Transport-Security header. [#17644](https://github.com/grafana/grafana/pull/17644), [@kylebrandt](https://github.com/kylebrandt)
  * **TablePanel**: fix annotations display. [#17646](https://github.com/grafana/grafana/pull/17646), [@ryantxu](https://github.com/ryantxu)

<!-- risk-assessed -->
