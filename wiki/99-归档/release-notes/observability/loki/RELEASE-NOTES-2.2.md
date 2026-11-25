---
title: loki v2.2 Release Notes
description: loki v2.2 Release Notes — Kubernetes 生产运维知识库
summary: loki v2.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v2.2 Release Notes 是什么
- 如何 loki v2.2 Release Notes
trigger_keywords:
- loki
- v2.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# loki v2.2 Release Notes

Source: [v2.2.1](https://github.com/grafana/loki/releases/tag/v2.2.1)

2.2.1 fixes several important bugs, it is recommended everyone running 2.2.0 upgrade to 2.2.1

2.2.1 also adds the `labelallow` pipeline stage in Promtail which lets an allowlist be created for what labels will be sent by Promtail to Loki.

* [3468](https://github.com/grafana/loki/pull/3468) **adityacs**: Support labelallow stage in Promtail
* [3502](https://github.com/grafana/loki/pull/3502) **cyriltovena**: Fixes a bug where unpack would mutate log line.
* [3540](https://github.com/grafana/loki/pull/3540) **cyriltovena**: Support for single step metric query.
* [3550](https://github.com/grafana/loki/pull/3550) **cyriltovena**: Fixes a bug in MatrixStepper when sharding queries.
* [3566](https://github.com/grafana/loki/pull/3566) **cyriltovena**: Properly release the ticker in Loki client.
* [3573](https://github.com/grafana/loki/pull/3573) **cyriltovena**: Fixes a race when using specific tenant and multi-client.

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:2.2.1"
$ docker pull "grafana/promtail:2.2.1"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.2.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
