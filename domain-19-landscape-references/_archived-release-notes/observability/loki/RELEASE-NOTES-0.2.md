---
title: loki v0.2 Release Notes
description: loki v0.2 Release Notes — Kubernetes 生产运维知识库
summary: loki v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- loki v0.2 Release Notes 是什么
- 如何 loki v0.2 Release Notes
trigger_keywords:
- loki
- v0.2
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



# loki v0.2 Release Notes

Source: [v0.2.0](https://github.com/grafana/loki/releases/tag/v0.2.0)

This is release `v0.2.0` of Loki. 

### Notable changes:

There were over 100 PR's merged since 0.1.0 was released. Please see the CHANGELOG](https://github.com/grafana/loki/blob/master/CHANGELOG.md) for a more complete list.

A few of the most notable improvements:

There were some significant improvements in memory handling and performance of Loki:

* **Loki**: [713](https://github.com/grafana/loki/pull/713) Storage memory improvement.
* **Loki**: [782](https://github.com/grafana/loki/pull/782) Performance improvement: Query storage by iterating through chunks in batches.
* **Loki**: [788](https://github.com/grafana/loki/pull/788) Querier timeouts.
* **Loki**: [794](https://github.com/grafana/loki/pull/794) Support ingester chunk transfer on shutdown.

Additionally, labels are now fetched from storage for the requested timeframe so you are no longer limited to only retrieving labels of what was still in memory:

* **Loki**:  [521](https://github.com/grafana/loki/pull/521) Query label values and names are now fetched from the store.

Please note this introduces a potential new issue if you're asking for labels across a large number of streams over a large timeframe, which causes a large amount of chunks to be loaded. A fix for this is currently being worked out.

Several additions were made to the log processing pipeline:

* **Pipeline**: [738](https://github.com/grafana/loki/pull/738) Added a template stage for manipulating label values.
* **Pipeline**: [732](https://github.com/grafana/loki/pull/732) Support for Unix timestamps.
* **Pipeline**: [760](https://github.com/grafana/loki/pull/760) Support timestamps without year.

A docker logging plugin was created to allow docker containers to send logs directly to Loki:

* **Docker-Plugin**: [663](https://github.com/grafana/loki/pull/663) Created a Docker logging driver plugin.

And starting with this release there are now multi-architecture docker containers and binaries:

* **Build**: [668](https://github.com/grafana/loki/pull/668),[762](https://github.com/grafana/loki/pull/762) Build multiple architecture containers.


### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Binary:
```bash
# download a binary (adapt app, os and arch as needed)
$ curl -fSL -o "/usr/local/bin/loki.gz" "https://github.com/grafana/loki/releases/download/v0.2.0/loki_linux_amd64.gz"
$ gunzip "/usr/local/bin/loki.gz"

# make sure it is executable
$ chmod a+x "/usr/local/bin/loki"
```

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v0.2.0"
$ docker pull "grafana/promtail:v0.2.0"
```