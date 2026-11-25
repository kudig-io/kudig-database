---
title: loki v2.6 Release Notes
description: loki v2.6 Release Notes — Kubernetes 生产运维知识库
summary: loki v2.6 Release Notes — Kubernetes 生产运维知识库
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
- loki v2.6 Release Notes 是什么
- 如何 loki v2.6 Release Notes
trigger_keywords:
- loki
- v2.6
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




# loki v2.6 Release Notes

Source: [v2.6.1](https://github.com/grafana/loki/releases/tag/v2.6.1)

Loki 2.6.1 is a patch fix release on [2.6.0](https://github.com/grafana/loki/releases/tag/v2.6.0)

### Notable changes:
- [PR 6658](https://github.com/grafana/loki/pull/6658) **JordanRushing**: Updated the versions of [dskit](https://github.com/grafana/dskit) and [memberlist](https://github.com/grafana/memberlist) to allow configuring cluster labels for memberlist. Cluster labels prevent mixing the members between two consistent hash rings of separate applications that are run on the same [[Kubernetes|Kubernetes]]es 集群配置最佳实践|Kubernetes cluster]].
- [PR 6681](https://github.com/grafana/loki/pull/6681) **MasslessParticle** Fixed an HTTP connection leak between the querier and the compactor when the log entry deletion feature is enabled.
- [PR 6583](https://github.com/grafana/loki/pull/6583) **MasslessParticle** Fixed noisy error messages when the log entry deletion feature is disabled for a tenant 



### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:2.6.1"
$ docker pull "grafana/promtail:2.6.1"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.6.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
