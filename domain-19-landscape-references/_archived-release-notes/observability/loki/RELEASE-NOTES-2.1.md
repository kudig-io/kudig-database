---
title: loki v2.1 Release Notes
description: loki v2.1 Release Notes — Kubernetes 生产运维知识库
summary: loki v2.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
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
- loki v2.1 Release Notes 是什么
- 如何 loki v2.1 Release Notes
trigger_keywords:
- loki
- v2.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- monitoring-basics
- logging-basics
---



# loki v2.1 Release Notes

Source: [v2.1.0](https://github.com/grafana/loki/releases/tag/v2.1.0)

# 🎆🎁Happy Holidays from the Loki team!🎁🎆

Please enjoy Loki 2.1.0 to welcome in the New Year!

This release contains a number of fixes, performance improvements and enhancements to the 2.0.0 release!

### Notable changes

#### [[Helm|Helm]] users read this!

The Helm charts have moved!

* [2720](https://github.com/grafana/loki/pull/2720) **torstenwalter**: Deprecate Charts as they have been moved

This was done to consolidate Grafana's helm charts for all Grafana projects in one place: https://github.com/grafana/helm-charts/

**From now moving forward, please use the new Helm repo url: https://grafana.github.io/helm-charts**

The charts in the Loki repo will soon be removed so please update your Helm repo to the new URL and submit your PR's over there as well

Special thanks to @torstenwalter, @unguiculus, and @scottrigby for their initiative and amazing work to make this happen!

Also go check out the microservices helm chart contributed by @unguiculus in the new repo!

#### Fluent bit plugin users read this!

Fluent bit officially supports Loki as an output plugin now! WoooHOOO!

However this created a naming conflict with our existing output plugin (the new native output uses the name `loki`) so we have renamed our plugin.

* [2974](https://github.com/grafana/loki/pull/2974) **hedss**: fluent-bit: Rename Fluent Bit plugin output name.

In time our plan is to deprecate and eliminate our output plugin in favor of the native Loki support. However until then you can continue using the plugin with the following change:

Old:

```
[Output]
    Name loki
```

New:

```
[Output]
    Name grafana-loki
```

#### Fixes

A lot of work went into 2.0 with a lot of new code and rewrites to existing, this introduced and uncovered some bugs which are fixed in 2.1:

* [2807](https://github.com/grafana/loki/pull/2807) **cyriltovena**: Fix error swallowed in the frontend.
* [2805](https://github.com/grafana/loki/pull/2805) **cyriltovena**: Improve pipeline stages ast errors.
* [2824](https://github.com/grafana/loki/pull/2824) **owen-d**: Fix/validate compactor config
* [2830](https://github.com/grafana/loki/pull/2830) **sandeepsukhani**: fix panic in ingester when not running with boltdb shipper while queriers does
* [2850](https://github.com/grafana/loki/pull/2850) **owen-d**: Only applies entry limits to non-SampleExprs.
* [2855](https://github.com/grafana/loki/pull/2855) **sandeepsukhani**: fix query intervals when running boltdb-shipper in single binary
* [2895](https://github.com/grafana/loki/pull/2895) **shokada**: Fix error 'Unexpected: ("$", "$") while parsing field definition'
* [2902](https://github.com/grafana/loki/pull/2902) **cyriltovena**: Fixes metric query issue with no grouping.
* [2901](https://github.com/grafana/loki/pull/2901) **cyriltovena**: Fixes a panic with the logql.NoopPipeline.
* [2913](https://github.com/grafana/loki/pull/2913) **cyriltovena**: Fixes logql.QueryType.
* [2917](https://github.com/grafana/loki/pull/2917) **cyriltovena**: Fixes race condition in tailer since logql v2.
* [2960](https://github.com/grafana/loki/pull/2960) **sandeepsukhani**: fix table deletion in table client for boltdb-shipper

#### Enhancements

A number of performance and resource improvements have been made as well!

* [2911](https://github.com/grafana/loki/pull/2911) **sandeepsukhani**: Boltdb shipper query readiness
* [2875](https://github.com/grafana/loki/pull/2875) **cyriltovena**: Labels computation LogQLv2
* [2927](https://github.com/grafana/loki/pull/2927) **cyriltovena**: Improve logql parser allocations.
* [2926](https://github.com/grafana/loki/pull/2926) **cyriltovena**: Cache label strings in ingester to improve memory usage.
* [2931](https://github.com/grafana/loki/pull/2931) **cyriltovena**: Only append tailed entries if needed.
* [2973](https://github.com/grafana/loki/pull/2973) **cyriltovena**: Avoid parsing labels when tailer is sending from a stream.
* [2959](https://github.com/grafana/loki/pull/2959) **cyriltovena**: Improve tailer matcher function.
* [2876](https://github.com/grafana/loki/pull/2876) **jkellerer**: LogQL: Add unwrap bytes() conversion function


#### Notable mentions

Thanks to @timbyr for adding an often requested feature, the ability to support environment variable expansion in config files!

* [2837](https://github.com/grafana/loki/pull/2837) **timbyr**: Configuration: Support environment expansion in configuration

Thanks to @huikang for adding a new docker-compose file for running Loki as microservices!

* [2740](https://github.com/grafana/loki/pull/2740) **huikang**: Deploy: add docker-compose cluster deploymentes 部署策略最佳实践|deployment]] file

### All Changes

For a full list of changes, please checkout the [CHANGELOG](https://github.com/grafana/loki/blob/master/CHANGELOG.md#210-20201223)

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:2.1.0"
$ docker pull "grafana/promtail:2.1.0"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.1.0/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```