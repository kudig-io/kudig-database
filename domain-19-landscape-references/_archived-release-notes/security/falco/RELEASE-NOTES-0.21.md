---
title: falco v0.21 Release Notes
description: falco v0.21 Release Notes — Kubernetes 生产运维知识库
summary: falco v0.21 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- falco v0.21 Release Notes 是什么
- 如何 falco v0.21 Release Notes
trigger_keywords:
- falco
- v0.21
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# [[Falco|falco]] v0.21 Release Notes

Source: [0.21.0](https://github.com/falcosecurity/falco/releases/tag/0.21.0)

Released on 2020-03-17

### Major Changes

* BREAKING CHANGE: the SYSDIG_BPF_PROBE environment variable is now just FALCO_BPF_PROBE (please update your systemd scripts or [[Kubernetes|kubernetes]] [[Deployments|deployments]]). [[#1050](https://github.com/falcosecurity/falco/pull/1050)]
* new: automatically publish deb packages (from git master branch) to public dev repository [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: automatically publish rpm packages (from git master branch) to public dev repository [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: automatically release deb packages (from git tags) to public repository [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: automatically release rpm packages (from git tags) to public repository [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: automatically publish docker images from master (master, master-slim, master-minimal) [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: automatically publish docker images from git tag (tag, tag-slim, tag-master, latest, latest-slim, latest-minimal) [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* new: sign packages with falcosecurity gpg key [[#1059](https://github.com/falcosecurity/falco/pull/1059)]

### Minor Changes

* new: falco_version_prerelease contains the number of commits since last tag on the master [[#1086](https://github.com/falcosecurity/falco/pull/1086)]
* docs: update branding [[#1074](https://github.com/falcosecurity/falco/pull/1074)]
* new(docker/event-generator): add example k8s resource files that allow running the event generator in a k8s cluster. [[#1088](https://github.com/falcosecurity/falco/pull/1088)]
* update: creating *-dev docker images using build arguments at build time [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* update: docker images use packages from the new repositories [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* update: docker image downloads old deb dependencies (gcc-6, gcc-5, binutils-2.30) from a new open repository [[#1059](https://github.com/falcosecurity/falco/pull/1059)]

### Bug Fixes

* fix(docker): updating `stable` and `local` images to run from `debian:stable` [[#1018](https://github.com/falcosecurity/falco/pull/1018)]
* fix(event-generator): the image used by the event generator deployment to `latest`. [[#1091](https://github.com/falcosecurity/falco/pull/1091)]
* fix: -t (to disable rules by certain tag) or -t (to only run rules with a certain tag) work now [[#1081](https://github.com/falcosecurity/falco/pull/1081)]
* fix: the falco driver now compiles on >= 5.4 kernels [[#1080](https://github.com/falcosecurity/falco/pull/1080)]
* fix: download falco packages which url contains character to encode - eg, `+` [[#1059](https://github.com/falcosecurity/falco/pull/1059)]
* fix(docker): use base name in docker-entrypoint.sh [[#981](https://github.com/falcosecurity/falco/pull/981)]

### Rule Changes

* rule(detect outbound connections to common miner pool ports): disabled by default [[#1061](https://github.com/falcosecurity/falco/pull/1061)]
* rule(macro net_miner_pool): add localhost and rfc1918 addresses as exception in the rule. [[#1061](https://github.com/falcosecurity/falco/pull/1061)]
* rule(change thread namespace): modify condition to detect suspicious container activity [[#974](https://github.com/falcosecurity/falco/pull/974)]

### Statistics

| Merged PRs          | Number |
|-------------------|---------|
| Not user-facing    | 7 |
| Release note        | 12 |
| Total                      | 19 |