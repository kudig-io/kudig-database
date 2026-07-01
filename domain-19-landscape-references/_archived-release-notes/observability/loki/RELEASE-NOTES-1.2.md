---
title: loki v1.2 Release Notes
description: loki v1.2 Release Notes — Kubernetes 生产运维知识库
summary: loki v1.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
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
- loki v1.2 Release Notes 是什么
- 如何 loki v1.2 Release Notes
trigger_keywords:
- loki
- v1.2
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



# loki v1.2 Release Notes

Source: [v1.2.0](https://github.com/grafana/loki/releases/tag/v1.2.0)

One week has passed since the last Loki release, and it's time for a new one!

### Notable Changes

We have continued our work making our API Prometheus-compatible. The key
changes centered around API compatibility are:

* [1370](https://github.com/grafana/loki/pull/1370) **slim-bean**: Change `/loki/api/v1/label` to `loki/api/v1/labels`
* [1381](https://github.com/grafana/loki/pull/1381) **owen-d**: application/x-www-form-urlencoded support

Meanwhile, @pstibrany has done great work ensuring that Loki handles hash
collisions properly:

* [1247](https://github.com/grafana/loki/pull/1247) **pstibrany**: pkg/ingester: handle labels mapping to the same fast fingerprint.

As always, the full list of changes can be found in the CHANGELOG](https://github.com/grafana/loki/blob/v1.2.0/CHANGELOG.md)!

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v1.2.0"
$ docker pull "grafana/promtail:v1.2.0"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v1.2.0/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```