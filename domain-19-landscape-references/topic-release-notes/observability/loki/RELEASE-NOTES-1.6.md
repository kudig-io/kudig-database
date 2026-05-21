---
title: loki v1.6 Release Notes
description: loki v1.6 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- loki v1.6 Release Notes 是什么
- 如何 loki v1.6 Release Notes
trigger_keywords:
- loki
- v1.6
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

# loki v1.6 Release Notes

Source: [v1.6.1](https://github.com/grafana/loki/releases/tag/v1.6.1)

This is a small release and only contains two fixes for Promtail:

* [2542](https://github.com/grafana/loki/pull/2542) **slim-bean**: Promtail: implement shutdown for the no-op server
* [2532](https://github.com/grafana/loki/pull/2532) **slim-bean**: Promtail: Restart the tailer if we fail to read and upate current position

The first only applies if you are running Promtail with both `--stdin` and `--server.disabled=true` flags.

The second is a minor rework to how Promtail handles a very specific error when attempting to read the size of a file and failing to do so.

Upgrading Promtail from 1.6.0 to 1.6.1 is only necessary if you have logs full of `msg="error getting tail position and/or size"`, 
the code changed in this release has been unchanged for a long time and we suspect very few people are seeing this issue.

No changes to any other components (Loki, Logcli, etc) are included in this release. 

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v1.6.1"
$ docker pull "grafana/promtail:v1.6.1"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v1.6.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```