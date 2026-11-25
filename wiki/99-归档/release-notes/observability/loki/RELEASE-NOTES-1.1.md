---
title: loki v1.1 Release Notes
description: loki v1.1 Release Notes — Kubernetes 生产运维知识库
summary: loki v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
- docker
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v1.1 Release Notes 是什么
- 如何 loki v1.1 Release Notes
trigger_keywords:
- loki
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- prometheus-basics
- monitoring-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# loki v1.1 Release Notes

Source: [v1.1.0](https://github.com/grafana/loki/releases/tag/v1.1.0)

It's been a busy 2 weeks since the 1.0.0 release and quite a few important PR's have been merged to Loki.

The most significant:

* [1322](https://github.com/grafana/loki/pull/1322) **rfratto**: Fix v1 label API to be Prometheus-compatible

Some might call this a **breaking change**, we are instead calling it a bug fix as our goal was to be [[Prometheus|prometheus]] compatible and we were not :smiley:  

**But please be aware if you are using the `/loki/api/v1/label` or `/loki/api/v1/label/<name>/values` the JSON result will be different in 1.1.0**

Old result:
```json
{
  "values": [
    "label1",
    "label2",
    "labeln"
  ]
}
```
New result:

```json
{
  "status": "success",
  "data": [
    "label1",
    "label2",
    "labeln"
  ]
}
``` 

**ALSO IMPORTANT**

* [1160](https://github.com/grafana/loki/pull/1160) **daixiang0**: replace gzip with zip

Binaries will now be zipped instead of gzipped as many people voiced their opinion that zip is likely to be installed on more systems by default.

**If you had existing automation to download and install binaries this will have to be updated to use zip instead of gzip**

## Notable Fixes and Improvements

* Broken version info in startup log message:
    
    [1095](https://github.com/grafana/loki/pull/1095) **pstibrany**: Makefile changes to allow easy builds with or without vendoring. Also fixes version bug for both cases.

* The hashing algorithm used to calculate the hash for a stream was creating hash collisions in some instances.  
**Please Note** this is just one part of the fix and is only in Promtail, the second part for Loki can be tracked [in PR1247](https://github.com/grafana/loki/pull/1247) which didn't quite make the cut for 1.1.0 and will be in 1.2.0:

    [1254](https://github.com/grafana/loki/pull/1254) **pstibrany**: pkg/promtail/client: Handle fingerprint hash collisions

* Thank you @putrasattvika for finding and fixing an important bug where logs were some logs were missed in a query shortly after a flush!

    [1299](https://github.com/grafana/loki/pull/1299) **putrasattvika**: storage: fix missing logs with batched chunk iterator

* Thank you @danieldabate for helping to again improve our API to be more Prometheus compatible:

    [1355](https://github.com/grafana/loki/pull/1355) **danieldabate**: HTTP API: Support duration and float formats for step parameter

* LogQL will support duration formats that are not typically handled by Go like [1d] or [1w]

    [1357](https://github.com/grafana/loki/pull/1357) **cyriltovena**: Supports same duration format in LogQL as Prometheus

The full list of changes can be found in the [CHANGELOG](https://github.com/grafana/loki/blob/v1.1.0/CHANGELOG.md#changes)

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:v1.1.0"
$ docker pull "grafana/promtail:v1.1.0"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v1.1.0/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
