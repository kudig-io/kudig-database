---
title: loki v0.4 Release Notes
description: loki v0.4 Release Notes — Kubernetes 生产运维知识库
summary: loki v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- prometheus
- grafana
- helm
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
- loki v0.4 Release Notes 是什么
- 如何 loki v0.4 Release Notes
trigger_keywords:
- loki
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- prometheus-basics
- monitoring-basics
- logging-basics
---



# loki v0.4 Release Notes

Source: [v0.4.0](https://github.com/grafana/loki/releases/tag/v0.4.0)

This is release `v0.4.0` of Loki. 

### EDIT 2019/10/25:

PR [948](https://github.com/grafana/loki/pull/948) introduces more limit configurations on Loki and **you may find the defaults too low for your installation** resulting in rate limiting on the clients writing to Loki which was not happening prior to v0.4.0. The limits can be configured higher to avoid this, [more info on limits can be found here](https://github.com/grafana/loki/blob/master/docs/configuration/README.md#limits_config).  

Limits are mostly useful for multi-tenant installations, however they can be useful in single tenant and even single binary installations as a mechanism to help keep Loki's resources constrained.  We are discussing if the current defaults are too low and/or if they should be removed entirely from the [[Helm|Helm]] installation.

Relevant issues #1200 and #1199

### Notable changes:

Please see the CHANGELOG](https://github.com/grafana/loki/blob/v0.4.0/CHANGELOG.md#040-2019-10-24) for full release notes.

* With PR [654](https://github.com/grafana/loki/pull/654) @cyriltovena added a really exciting new capability to Loki, a [[Prometheus|Prometheus]] compatible API with support for running metric style queries against your logs! [Take a look at how to write metric queries for logs](https://github.com/grafana/loki/blob/master/docs/logql.md#counting-logs)
    > PLEASE NOTE: To use metric style queries in the current Grafana release 6.4.x you will need to add Loki as a Prometheus datasource in addition to having it as a Log datasource and you will have to select the correct source for querying logs vs metrics, coming soon Grafana will support both logs and metric queries directly to the Loki datasource!
* PR [1022](https://github.com/grafana/loki/pull/1022) (and a few others) @joe-elliott added a new set of HTTP endpoints in conjunction with the work @cyriltovena to create a Prometheus compatible API as well as improve how labels/timestamps are handled
    > IMPORTANT: The new `/api/v1/*` endpoints contain breaking changes on the query paths (push path is unchanged) Eventually the `/api/prom/*` endpoints will be removed
* PR [847](https://github.com/grafana/loki/pull/847) owes a big thanks to @cosmo0920 for contributing his Fluent Bit go plugin, now loki has Fluent Bit plugin support!!

* PR [982](https://github.com/grafana/loki/pull/982) was a couple weeks of painstaking work by @rfratto for a much needed improvement to Loki's docs! [Check them out!](https://github.com/grafana/loki/tree/master/docs) 

* PR [980](https://github.com/grafana/loki/pull/980) by @sh0rez improved how flags and config file's are loaded to honor a more traditional order of precedence:
    1. Defaults
    2. Config file
    3. User-supplied flag values (command line arguments)
    > PLEASE NOTE: This is potentially a breaking change if you were passing command line arguments that also existed in a config file in which case the order they are given priority now has changed!
  
* PR [1062](https://github.com/grafana/loki/pull/1062) and [1089](https://github.com/grafana/loki/pull/1089) have moved Loki from Dep to Go Modules and to Go 1.13

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Binary:
```bash
# download a binary (adapt app, os and arch as needed)
$ curl -fSL -o "/usr/local/bin/loki.gz" "https://github.com/grafana/loki/releases/download/v0.4.0/loki-linux-amd64.gz"
$ gunzip "/usr/local/bin/loki.gz"

# make sure it is executable
$ chmod a+x "/usr/local/bin/loki"
```

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:v0.4.0"
$ docker pull "grafana/promtail:v0.4.0"
```