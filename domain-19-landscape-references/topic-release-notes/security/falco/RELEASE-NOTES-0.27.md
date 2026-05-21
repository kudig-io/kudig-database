---
title: falco v0.27 Release Notes
description: falco v0.27 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- calico
- docker
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- falco v0.27 Release Notes 是什么
- 如何 falco v0.27 Release Notes
trigger_keywords:
- falco
- v0.27
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- cni-basics
---

# falco v0.27 Release Notes

Source: [0.27.0](https://github.com/falcosecurity/falco/releases/tag/0.27.0)


Released on 2021-01-18

| Packages | Download                                                                                                                                               |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------ |
| rpm      | [![rpm](https://img.shields.io/badge/Falco-0.27.0-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/rpm/falco-0.27.0-x86_64.rpm)        |
| deb      | [![deb](https://img.shields.io/badge/Falco-0.27.0-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/deb/stable/falco-0.27.0-x86_64.deb) |
| tgz      | [![tgz](https://img.shields.io/badge/Falco-0.27.0-%2300aec7?style=flat-square)](https://dl.bintray.com/falcosecurity/bin/x86_64/falco-0.27.0-x86_64.deb) |

| Images                                                                       |
| ---------------------------------------------------------------------------- |
| `docker pull docker.io/falcosecurity/falco:0.27.0`                           |
| `docker pull public.ecr.aws/falcosecurity/falco:0.27.0`                      |
| `docker pull docker.io/falcosecurity/falco-driver-loader:0.27.0`             |
| `docker pull docker.io/falcosecurity/falco-no-driver:0.27.0`                 |


### Major Changes

* new: Added falco engine version to grpc version service [[#1507](https://github.com/falcosecurity/falco/pull/1507)] - [@nibalizer](https://github.com/nibalizer)
* BREAKING CHANGE: Users who run Falco without a config file will be unable to do that any more, Falco now expects a configuration file to be passed all the times. Developers may need to adjust their processes. [[#1494](https://github.com/falcosecurity/falco/pull/1494)] - [@nibalizer](https://github.com/nibalizer)
* new: asynchronous outputs implementation, outputs channels will not block event processing anymore [[#1451](https://github.com/falcosecurity/falco/pull/1451)] - [@leogr](https://github.com/leogr)
* new: slow outputs detection [[#1451](https://github.com/falcosecurity/falco/pull/1451)] - [@leogr](https://github.com/leogr)
* new: `output_timeout` config option for slow outputs detection [[#1451](https://github.com/falcosecurity/falco/pull/1451)] - [@leogr](https://github.com/leogr)


### Minor Changes

* build: bump b64 to v2.0.0.1 [[#1441](https://github.com/falcosecurity/falco/pull/1441)] - [@fntlnz](https://github.com/fntlnz)
* rules(macro container_started): re-use `spawned_process` macro inside `container_started` macro [[#1449](https://github.com/falcosecurity/falco/pull/1449)] - [@leodido](https://github.com/leodido)
* docs: reach out documentation [[#1472](https://github.com/falcosecurity/falco/pull/1472)] - [@fntlnz](https://github.com/fntlnz)
* docs: Broken outputs.proto link [[#1493](https://github.com/falcosecurity/falco/pull/1493)] - [@deepskyblue86](https://github.com/deepskyblue86)
* docs(README.md): correct broken links [[#1506](https://github.com/falcosecurity/falco/pull/1506)] - [@leogr](https://github.com/leogr)
* docs(proposals): Exceptions handling proposal [[#1376](https://github.com/falcosecurity/falco/pull/1376)] - [@mstemm](https://github.com/mstemm)
* docs: fix a broken link of README [[#1516](https://github.com/falcosecurity/falco/pull/1516)] - [@oke-py](https://github.com/oke-py)
* docs: adding the kubernetes privileged use case to use cases [[#1484](https://github.com/falcosecurity/falco/pull/1484)] - [@fntlnz](https://github.com/fntlnz)
* rules(Mkdir binary dirs): Adds exe_running_docker_save as an exception as this rules can be triggerred when a container is created. [[#1386](https://github.com/falcosecurity/falco/pull/1386)] - [@jhwbarlow](https://github.com/jhwbarlow)
* rules(Create Hidden Files): Adds exe_running_docker_save as an exception as this rules can be triggerred when a container is created. [[#1386](https://github.com/falcosecurity/falco/pull/1386)] - [@jhwbarlow](https://github.com/jhwbarlow)
* docs(.circleci): welcome Jonah (Amazon) as a new Falco CI maintainer [[#1518](https://github.com/falcosecurity/falco/pull/1518)] - [@leodido](https://github.com/leodido)
* build: falcosecurity/falco:master also available on the AWS ECR Public registry [[#1512](https://github.com/falcosecurity/falco/pull/1512)] - [@leodido](https://github.com/leodido)
* build: falcosecurity/falco:latest also available on the AWS ECR Public registry [[#1512](https://github.com/falcosecurity/falco/pull/1512)] - [@leodido](https://github.com/leodido)
* update: gRPC clients can now subscribe to drop alerts via gRCP API [[#1451](https://github.com/falcosecurity/falco/pull/1451)] - [@leogr](https://github.com/leogr)
* macro(allowed_k8s_users): exclude cloud-controller-manage to avoid false positives on k3s [[#1444](https://github.com/falcosecurity/falco/pull/1444)] - [@fntlnz](https://github.com/fntlnz)


### Bug Fixes

* fix(userspace/falco): use given priority in falco_outputs::handle_msg() [[#1450](https://github.com/falcosecurity/falco/pull/1450)] - [@leogr](https://github.com/leogr)
* fix(userspace/engine): free formatters, if any [[#1447](https://github.com/falcosecurity/falco/pull/1447)] - [@leogr](https://github.com/leogr)
* fix(scripts/falco-driver-loader): lsmod usage [[#1474](https://github.com/falcosecurity/falco/pull/1474)] - [@dnwe](https://github.com/dnwe)
* fix: a bug that prevents Falco driver to be consumed by many Falco instances in some circumstances [[#1485](https://github.com/falcosecurity/falco/pull/1485)] - [@leodido](https://github.com/leodido)
* fix: set `HOST_ROOT=/host` environment variable for the `falcosecurity/falco-no-driver` container image by default [[#1492](https://github.com/falcosecurity/falco/pull/1492)] - [@leogr](https://github.com/leogr)


### Rule Changes

* rule(list user_known_change_thread_namespace_binaries): add crio and multus to the list [[#1501](https://github.com/falcosecurity/falco/pull/1501)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(Container Run as Root User): new rule created [[#1500](https://github.com/falcosecurity/falco/pull/1500)] - [@Kaizhe](https://github.com/Kaizhe)
* rule(Linux Kernel Module injection detected): adds a new rule that detects when an LKM module is injected using `insmod` from a container (typically used by rootkits looking to obfuscate their behavior via kernel hooking). [[#1478](https://github.com/falcosecurity/falco/pull/1478)] - [@d1vious](https://github.com/d1vious)
* rule(macro multipath_writing_conf): create and use the macro [[#1475](https://github.com/falcosecurity/falco/pull/1475)] - [@nmarier-coveo](https://github.com/nmarier-coveo)
* rule(list falco_privileged_images): add calico/node without registry prefix to prevent false positive alerts [[#1457](https://github.com/falcosecurity/falco/pull/1457)] - [@czunker](https://github.com/czunker)
* rule(Full K8s Administrative Access): use the right list of admin users (fix) [[#1454](https://github.com/falcosecurity/falco/pull/1454)] - [@mstemm](https://github.com/mstemm)


### Non user-facing changes

* chore(cmake): remove unnecessary whitespace patch [[#1522](https://github.com/falcosecurity/falco/pull/1522)] - [@leogr](https://github.com/leogr)
* remove stale bot in favor of the new lifecycle bot [[#1490](https://github.com/falcosecurity/falco/pull/1490)] - [@leodido](https://github.com/leodido)
* chore(cmake): mark some variables as advanced [[#1496](https://github.com/falcosecurity/falco/pull/1496)] - [@deepskyblue86](https://github.com/deepskyblue86)
* chore(cmake/modules): avoid useless rebuild [[#1495](https://github.com/falcosecurity/falco/pull/1495)] - [@deepskyblue86](https://github.com/deepskyblue86)
* build: BUILD_BYPRODUCTS for civetweb [[#1489](https://github.com/falcosecurity/falco/pull/1489)] - [@fntlnz](https://github.com/fntlnz)
* build: remove duplicate item from FALCO_SOURCES [[#1480](https://github.com/falcosecurity/falco/pull/1480)] - [@leodido](https://github.com/leodido)
* build: make our integration tests report clear steps for CircleCI UI [[#1473](https://github.com/falcosecurity/falco/pull/1473)] - [@fntlnz](https://github.com/fntlnz)
* further improvements outputs impl.  [[#1443](https://github.com/falcosecurity/falco/pull/1443)] - [@leogr](https://github.com/leogr)
* fix(test): make integration tests properly fail [[#1439](https://github.com/falcosecurity/falco/pull/1439)] - [@leogr](https://github.com/leogr)
* Falco outputs refactoring [[#1412](https://github.com/falcosecurity/falco/pull/1412)] - [@leogr](https://github.com/leogr)



### Statistics

| Merged PRs      | Number |
| --------------- | ------ |
| Not user-facing | 10     |
| Release note    | 30     |
| Total           | 40     |

