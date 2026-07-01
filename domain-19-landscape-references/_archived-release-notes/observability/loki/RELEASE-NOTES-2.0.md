---
title: loki v2.0 Release Notes
description: loki v2.0 Release Notes — Kubernetes 生产运维知识库
summary: loki v2.0 Release Notes — Kubernetes 生产运维知识库
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
- loki v2.0 Release Notes 是什么
- 如何 loki v2.0 Release Notes
trigger_keywords:
- loki
- v2.0
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



# loki v2.0 Release Notes

Source: [v2.0.1](https://github.com/grafana/loki/releases/tag/v2.0.1)

2.0.1 is a special release, it only exists to add the v3 support to Loki's chunk format.

**There is no reason to upgrade from 2.0.0 to 2.0.1**

This chunk version is internal to Loki and not configurable, and in a future version v3 will become the default (Likely 2.2.0).

We are creating this to enable users to roll back from a future release which was writing v3 chunks, back as far as 2.0.0 and still be able to read chunks.

This is mostly a safety measure to help if someone upgrades from 2.0.0 and skips versions to a future version which is writing v3 chunks and they encounter an issue which they would like to roll back. They would be able to then roll back to 2.0.1 and still read v3 chunks.

It should be noted this does not help anyone upgrading from a version older than 2.0.0, that is you should at least upgrade to 2.0.0 before going to a newer version if you are on a version older than 2.0.0.

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:2.0.1"
$ docker pull "grafana/promtail:2.0.1"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.0.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```