---
title: falco v0.20 Release Notes
description: falco v0.20 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- cilium
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.20 Release Notes 是什么
- 如何 falco v0.20 Release Notes
trigger_keywords:
- falco
- v0.20
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cilium-basics
created: "2026-05-23"
---

# [[Falco|falco]] v0.20 Release Notes

Source: [0.20.0](https://github.com/falcosecurity/falco/releases/tag/0.20.0)

Released on 2020-02-24

### Major Changes

* fix: memory leak introduced in 0.18.0 happening while using json events and the [[Kubernetes|kubernetes]] audit endpoint [[#1041](https://github.com/falcosecurity/falco/pull/1041)]
* new: [[gRPC|grpc]] version api [[#872](https://github.com/falcosecurity/falco/pull/872)]


### Bug Fixes

* fix: the base64 output format (-b) now works with both json and normal output. [[#1033](https://github.com/falcosecurity/falco/pull/1033)]
* fix: version follows semver 2 bnf [[#872](https://github.com/falcosecurity/falco/pull/872)]

### Rule Changes

* rule(write below etc): add "dsc_host" as a ms oms program [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(write below etc): let mcafee write to /etc/cma.d  [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(write below etc): let avinetworks supervisor write some ssh cfg [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(write below etc): alow writes to /etc/pki from openshift [[Secrets|secrets]] dir [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(write below root): let runc write to /exec.fifo [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(change thread namespace): let cilium-cni change namespaces [[#1028](https://github.com/falcosecurity/falco/pull/1028)]
* rule(run shell untrusted): let puma reactor spawn shells [[#1028](https://github.com/falcosecurity/falco/pull/1028)]

### Statistics

| Merged PRs          | Number |
|-------------------|---------|
| Not user-facing    | 5 |
| Release note        | 4 |
| Total                      | 9 |
