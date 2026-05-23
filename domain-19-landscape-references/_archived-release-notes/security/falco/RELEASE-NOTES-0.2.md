---
title: falco v0.2 Release Notes
description: falco v0.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- falco
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.2 Release Notes 是什么
- 如何 falco v0.2 Release Notes
trigger_keywords:
- falco
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# [[Falco|falco]] v0.2 Release Notes

Source: [v0.2.0](https://github.com/falcosecurity/falco/releases/tag/v0.2.0)

Released 2016-06-09

For full handling of setsid system calls and session id tracking using `proc.sname`, falco requires a sysdig version >= 0.10.0.

### Major Changes
- Add TravisCI regression tests. Testing involves a variety of positive, negative, and informational trace files with both plain and json output. [[#76](https://github.com/draios/falco/pull/76)] [[#83](https://github.com/draios/falco/pull/83)]
- Fairly big rework of ruleset to improve coverage, reduce false positives, and handle installation environments effectively [[#83](https://github.com/draios/falco/pull/83)] [[#87](https://github.com/draios/falco/pull/87)]
- Not directly a code change, but mentioning it here--the Wiki has now been populated with an initial set of articles, migrating content from the README and adding detail when necessary. [[#90](https://github.com/draios/falco/pull/90)]

### Minor Changes
- Improve JSON output to include the rule name, full output string, time, and severity [[#89](https://github.com/draios/falco/pull/89)]

### Bug Fixes
- Improve CMake quote handling [[#84](https://github.com/draios/falco/pull/84)]
- Remove unnecessary NULL check of a delete [[#85](https://github.com/draios/falco/pull/85)]
