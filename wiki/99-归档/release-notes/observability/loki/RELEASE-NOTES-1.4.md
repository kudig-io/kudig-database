---
title: loki v1.4 Release Notes
description: loki v1.4 Release Notes — Kubernetes 生产运维知识库
summary: loki v1.4 Release Notes — Kubernetes 生产运维知识库
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
- loki v1.4 Release Notes 是什么
- 如何 loki v1.4 Release Notes
trigger_keywords:
- loki
- v1.4
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




# loki v1.4 Release Notes

Source: [v1.4.1](https://github.com/grafana/loki/releases/tag/v1.4.1)

This is release `v1.4.1` of Loki.

### Important Notes

Please read the **Important Notes** section for v1.4.0

Mostly be sure to check out the [upgrading page](https://github.com/grafana/loki/blob/master/docs/operations/upgrade.md#140), everything for 1.4.0 applies to 1.4.1

### Notable changes:

We realized after the release last week that piping data into promtail was not working on Linux or Windows, this should fix this issue for both platforms:

* [1893](https://github.com/grafana/loki/pull/1893) **cyriltovena**: Removes file size check for pipe, not provided by linux.

Also thanks to @dottedmag for providing this fix for Fluent Bit!

* [1890](https://github.com/grafana/loki/pull/1890) **dottedmag**: fluentbit: JSON encoding: avoid base64 encoding of []byte inside other slices


### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:1.4.1"
$ docker pull "grafana/promtail:1.4.1"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v1.4.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
