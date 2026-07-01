---
title: loki v1.0 Release Notes
description: loki v1.0 Release Notes — Kubernetes 生产运维知识库
summary: loki v1.0 Release Notes — Kubernetes 生产运维知识库
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
- loki v1.0 Release Notes 是什么
- 如何 loki v1.0 Release Notes
trigger_keywords:
- loki
- v1.0
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



# loki v1.0 Release Notes

Source: [v1.0.0](https://github.com/grafana/loki/releases/tag/v1.0.0)


🎉 Nearly a year since Loki was announced at KubeCon in Seattle 2018 we are very excited to announce the 1.0.0 release of Loki! 🎉

A lot has happened since the announcement, the project just recently passed 1000 commits by 138 contributors over 700+ PR's accumulating over 7700 GitHub stars!

Internally at Grafana Labs we have been using Loki to monitor all of our infrastructure and ingest around 1.5TB/10 billion log lines a day. Since the v0.2.0 release we have found Loki to be reliable and stable in our environments.

We are comfortable with the state of the project in our production environments and think it's time to promote Loki to a non-beta release to communicate to everyone that they should feel comfortable using Loki in their production environments too.
API Stability

With the 1.0.0 release our intent is to try to follow Semver rules regarding stability with some aspects of Loki, focusing mainly on the operating experience of Loki as an application. That is to say we are not planning any major changes to the HTTP API, and anything breaking would likely be accompanied by a major release with backwards compatibility support.

We are currently NOT planning on maintaining Go API stability with this release, if you are importing Loki as a library you should be prepared for any kind of change, including breaking, even in minor or bugfix releases.

Loki is still a young and active project and there might be some breaking config changes in non-major releases, rest assured this will be clearly communicated and backwards or overlapping compatibility will be provided if possible.

The full list of changes for this release can be found in the CHANGELOG](https://github.com/grafana/loki/blob/v1.0.0/CHANGELOG.md#changes)

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v1.0.0"
$ docker pull "grafana/promtail:v1.0.0"
```

#### Binary:

Choose from the assets below for the application and architecture matching your system.

Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
# curl -L "https://github.com/grafana/loki/releases/download/v1.0.0/loki-linux-amd64.gz

# extract the binary
$ gunzip "loki-linux-amd64.gz"

# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```