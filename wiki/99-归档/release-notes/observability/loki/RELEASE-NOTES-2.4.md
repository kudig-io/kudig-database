---
title: loki v2.4 Release Notes
description: loki v2.4 Release Notes — Kubernetes 生产运维知识库
summary: loki v2.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- scheduler
- grafana
- docker
- redis
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v2.4 Release Notes 是什么
- 如何 loki v2.4 Release Notes
trigger_keywords:
- loki
- v2.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- monitoring-basics
- redis-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# loki v2.4 Release Notes

Source: [v2.4.2](https://github.com/grafana/loki/releases/tag/v2.4.2)

## Loki 2.4.2

Loki 2.4.2 is a patch fix release on 2.4.x

### Defaults changes

2.4.2 makes the following changes to Loki defaults to improve usability, see [PR 5077](https://github.com/grafana/loki/pull/5077):

| config | new default | old default |
| --- | --- | --- |
| parallelise_shardable_queries | true | false |
| split_queries_by_interval | 30m | 0s |
| query_ingesters_within | 3h | 0s |
| max_chunk_age | 2h | 1h |
| max_concurrent | 10 | 20 |

### Bug fixes

2.4.2 fixes these bugs:

- [PR 4968](https://github.com/grafana/loki/pull/4968) **trevorwhitney**: Fixes a bug in which querying ingesters wrongly returns a ruler,
causing the internal server error `code = Unimplemented`.
- [PR 4875](https://github.com/grafana/loki/pull/4875) **trevorwhitney**: Honor the replication factor specified in the common configuration block when `memberlist` is the consistent hash ring store.
- [PR 4792](https://github.com/grafana/loki/pull/4792) **AndreZiviani**: Corrects the default values of configuration options in the documentation for:
    - `scheduler_dns_lookup_period` 
    - `min_ready_duration` 
    - `final_sleep` 
    - `max_transfer_retries` 
    - `chunk_retain_period` 
    - `chunk_target_size` 
    - `batch_size` 
    - `timeout` (for Redis requests) 

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:2.4.2"
$ docker pull "grafana/promtail:2.4.2"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.4.2/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
