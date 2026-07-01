---
title: falco v0.4 Release Notes
description: falco v0.4 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.4 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
- operator
- webhook
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.4 Release Notes 是什么
- 如何 falco v0.4 Release Notes
trigger_keywords:
- falco
- v0.4
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Falco|falco]] v0.4 Release Notes

Source: [0.4.0](https://github.com/falcosecurity/falco/releases/tag/0.4.0)

Released 2016-10-25

As falco depends heavily on sysdig, many changes here were actually made to sysdig and pulled in as a part of the build process. Issues/PRs starting with `sysdig/#XXX` are sysdig changes.

### Major Changes
- Improved visibility into containers:
  - New filter `container.privileged` to match containers running in privileged mode [[sysdig/#655](https://github.com/draios/sysdig/pull/655)] [[sysdig/#658](https://github.com/draios/sysdig/pull/658)]
  - New rules utilizing privileged state [[#121](https://github.com/draios/falco/pull/121)]
  - New filters `container.mount*` to match container mount points [[sysdig/#655](https://github.com/draios/sysdig/pull/655)]
  - New rules utilizing container mount points [[#120](https://github.com/draios/falco/pull/120)]  
  - New filter `container.image.id` to match container image id [[sysdig/#661](https://github.com/draios/sysdig/pull/661)]
- Improved visibility into orchestration environments:
  - New k8s.deployment.\* and k8s.rs.\* filters to support latest [[Kubernetes|kubernetes]] features [[sysdg/#dbf9b5c](https://github.com/draios/sysdig/commit/dbf9b5c893d49f945c59684b4effe5700d730973)]  
  - Rule changes to avoid FPs when monitoring k8s environments [[#138](https://github.com/draios/falco/pull/138)]
  - Add new options `-pc`/`-pk`/`-pm`/`-k`/`-m` analogous to sysdig command line options. These options pull metadata information from k8s/mesos servers and adjust default falco notification outputs to contain container/orchestration information when applicable. [[#131](https://github.com/draios/falco/pull/131)] [[#134](https://github.com/draios/falco/pull/134)]
- Improved ability to work with file pathnames:
  - Added `glob` operator for strings, works as classic shell glob path matcher [[sysdig/#653](https://github.com/draios/sysdig/pull/653)]
  - Added `pmatch` operator to efficiently test a subject pathname against a set of target pathnames, to see if the subject is a prefix of any target [[sysdig/#660](https://github.com/draios/sysdig/pull/660)] [[#125](https://github.com/draios/falco/pull/125)]

### Minor Changes
- Add an event generator program that simulates suspicious activity that can be detected by falco. This is also available as a docker image [sysdig/falco-event-generator](https://hub.docker.com/r/sysdig/falco-event-generator/). [[#113](https://github.com/draios/falco/pull/113)] [[#132](https://github.com/draios/falco/pull/132)]
- Changed rule names to be human readable [[#116](https://github.com/draios/falco/pull/116)]
- Add Copyright notice to all source files [[#126](https://github.com/draios/falco/pull/126)]
- Changes to docker images to make it easier to massage JSON output for webhooks [[#133](https://github.com/draios/falco/pull/133)]
- When run with `-v`, print statistics on the number of events processed and dropped [[#139](https://github.com/draios/falco/pull/139)]
- Add ability to write trace files with `-w`. This can be useful to write a trace file in parallel with live event monitoring so you can reproduce it later. [[#140](https://github.com/draios/falco/pull/140)]
- All rules can now take an optional `enabled` flag. With `enabled: false`, a rule will not be loaded or run against events. By default all rules are enabled [[#119](https://github.com/draios/falco/pull/119)]

### Bug Fixes
- Fixed rule FPs related to docker's `docker`/`dockerd` split in 1.12 [[#112](https://github.com/draios/falco/pull/112)]
- Fixed rule FPs related to sysdigcloud agent software [[#141](https://github.com/draios/falco/pull/141)]
- Minor changes to node.js example to avoid falco false positives [[#111](https://github.com/draios/falco/pull/111/)]
- Fixed regression that broke configurable outputs [[#117](https://github.com/draios/falco/pull/117)]. This was not broken in 0.3.0, just between 0.3.0 and 0.4.0.
- Fixed a lua stack leak that could cause problems when matching millions of events against a large set of rules [[#123](https://github.com/draios/falco/pull/123)]
- Update docker files to reflect changes to `debian:unstable` docker image [[#124](https://github.com/draios/falco/pull/124)]
- Fixed logic for detecting config files to ensure config files in `/etc/falco.yaml` are properly detected  [[#135](https://github.com/draios/falco/pull/135)] [[#136](https://github.com/draios/falco/pull/136)]
- Don't alert on falco spawning a shell for program output notifications [[#137](https://github.com/draios/falco/pull/137)]
