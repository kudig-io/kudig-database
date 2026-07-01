---
title: loki v0.3 Release Notes
description: loki v0.3 Release Notes — Kubernetes 生产运维知识库
summary: loki v0.3 Release Notes — Kubernetes 生产运维知识库
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
- loki v0.3 Release Notes 是什么
- 如何 loki v0.3 Release Notes
trigger_keywords:
- loki
- v0.3
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



# loki v0.3 Release Notes

Source: [v0.3.0](https://github.com/grafana/loki/releases/tag/v0.3.0)

This is release `v0.3.0` of Loki. 

### Notable changes:

Please see the CHANGELOG](https://github.com/grafana/loki/blob/master/CHANGELOG.md#030-2019-08-16) for full release notes.

Loki sees readiness probes added for the queriers and distributors (for running Loki in microservices mode), some cleanup and improvement to the live tailing code, fix panic when ingesters are removed while being live tailed, and a couple bugs around chunk handling.

The promtail amd64 image now supports reading Systemd journal files.  This has been a struggle as their only exists C libraries for reading the journal file which requires using CGO and an image with systemd packages.  For now we have switched to a debian image for promtail which has the necessary systemd files.  Currently however cross compiling for ARM is too much of a struggle and we have deferred this for when we switch to the native image building with the drone.io CI system, at which point we will have both amd64 and arm promtail images which can read the Systemd journal.

As just mentioned there has been a big effort to offload building images from circle ci to drone.io where we have access to native arm containers for building arm images without cross compiling.  Drone also seems to be building much faster.  Currently we are building in both CI systems but soon will switch the image building to drone.

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Binary:
```bash
# download a binary (adapt app, os and arch as needed)
$ curl -fSL -o "/usr/local/bin/loki.gz" "https://github.com/grafana/loki/releases/download/v0.3.0/loki_linux_amd64.gz"
$ gunzip "/usr/local/bin/loki.gz"

# make sure it is executable
$ chmod a+x "/usr/local/bin/loki"
```

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v0.3.0"
$ docker pull "grafana/promtail:v0.3.0"
```